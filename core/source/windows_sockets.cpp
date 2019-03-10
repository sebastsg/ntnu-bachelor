#include "windows_sockets.hpp"

#if PLATFORM_WINDOWS

#define WS_PRINT_ERROR(ERR)      print_winsock_error(ERR, __FUNCSIG__, __LINE__, 0)
#define WS_PRINT_ERROR_X(ERR, X) print_winsock_error(ERR, __FUNCSIG__, __LINE__, X)
#define WS_PRINT_LAST_ERROR()    print_winsock_error(WSAGetLastError(), __FUNCSIG__, __LINE__, 0)
#define WS_PRINT_LAST_ERROR_X(X) print_winsock_error(WSAGetLastError(), __FUNCSIG__, __LINE__, X)

namespace no {

DWORD io_port_thread(HANDLE io_port, int thread_num);

static winsock_state winsock;

static void print_winsock_error(int error, const std::string& funcsig, int line, int log) {
	int language = MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT);
	char message[256] = {};
	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, error, language, message, 256, nullptr);
	CRITICAL_X(log, "WSA Error " << error << " on line " << line << " in " << funcsig << "\n" << message);
}

static void create_completion_port() {
	// it is possible to create more threads than specified below
	// but only max this amount of threads can run simultaneously
	// 0 -> number of CPUs -- not supported in the for loop below
	const int thread_count = 2;
	winsock.io_port = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, thread_count);
	if (!winsock.io_port) {
		CRITICAL("Failed to create I/O completion port. Error: " << GetLastError());
		return;
	}
	for (int i = 0; i < thread_count; i++) {
		winsock.threads.emplace_back([i] {
			io_port_thread(winsock.io_port, i);
		});
	}
}

static void destroy_completion_port() {
	if (winsock.io_port == INVALID_HANDLE_VALUE) {
		return;
	}
	iocp_close_data close_io;
	for (auto& thread : winsock.threads) {
		PostQueuedCompletionStatus(winsock.io_port, 0, 0, &close_io.overlapped);
	}
	// join all threads. each should receive their own close event
	for (auto& thread : winsock.threads) {
		if (thread.joinable()) {
			thread.join();
		}
	}
	winsock.threads.clear();
	CloseHandle(winsock.io_port);
	winsock.io_port = INVALID_HANDLE_VALUE;
}

winsock_socket::winsock_socket() {
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_family = AF_INET;
	addr.sin_family = hints.ai_family;
}

static void load_extensions(SOCKET handle) {
	if (winsock.AcceptEx) {
		return;
	}
	// AcceptEx
	DWORD bytes = 0;
	DWORD code = SIO_GET_EXTENSION_FUNCTION_POINTER;
	GUID guid = WSAID_ACCEPTEX;
	WSAIoctl(handle, code, &guid, sizeof(guid), &winsock.AcceptEx, sizeof(winsock.AcceptEx), &bytes, nullptr, nullptr);
	// GetAcceptExSockaddrs
	bytes = 0;
	guid = WSAID_GETACCEPTEXSOCKADDRS;
	WSAIoctl(handle, code, &guid, sizeof(guid), &winsock.GetAcceptExSockaddrs, sizeof(winsock.GetAcceptExSockaddrs), &bytes, nullptr, nullptr);
}

static int update_accept_context(SOCKET client, SOCKET listener) {
	return setsockopt(client, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (char*)&listener, sizeof(listener));
}

static void get_accept_sockaddrs(iocp_accept_data& data) {
	if (!winsock.GetAcceptExSockaddrs) {
		return;
	}
	// todo: at the moment we aren't doing anything with the result here
	DWORD addr_size = iocp_accept_data::padded_addr_size;
	sockaddr* local = nullptr;
	sockaddr* remote = nullptr;
	int local_size = sizeof(local);
	int remote_size = sizeof(remote);
	winsock.GetAcceptExSockaddrs(data.buffer.buf, 0, addr_size, addr_size, &local, &local_size, &remote, &remote_size);
}

static void destroy_socket(int id) {
	auto& socket = winsock.sockets[id];
	if (socket.handle == INVALID_SOCKET) {
		return;
	}
	int success = closesocket(socket.handle);
	if (success == SOCKET_ERROR) {
		WS_PRINT_LAST_ERROR();
		return;
	}
	socket.handle = INVALID_SOCKET;
	for (auto send : socket.io.send) {
		delete send;
	}
	for (auto receive : socket.io.receive) {
		delete receive;
	}
	for (auto accept : socket.io.accept) {
		delete accept;
	}
	winsock.sockets[id] = {};
}

static bool create_socket(int id) {
	auto& socket = winsock.sockets[id];
	if (socket.handle != INVALID_SOCKET) {
		return true;
	}
	socket.handle = WSASocketW(socket.hints.ai_family, socket.hints.ai_socktype, socket.hints.ai_protocol, nullptr, 0, WSA_FLAG_OVERLAPPED);
	if (socket.handle == INVALID_SOCKET) {
		WS_PRINT_LAST_ERROR();
		return false;
	}
	CreateIoCompletionPort((HANDLE)socket.handle, winsock.io_port, (ULONG_PTR)id, 0);
	return true;
}

static bool connect_socket(int id) {
	auto& socket = winsock.sockets[id];
	create_socket(id);
	int success = ::connect(socket.handle, (SOCKADDR*)&socket.addr, socket.addr_size);
	if (success != 0) {
		WS_PRINT_LAST_ERROR();
		return false;
	}
	socket.connected = true;
	return true;
}

static bool connect_socket(int id, const std::string& address, int port) {
	auto& socket = winsock.sockets[id];
	addrinfo* result = nullptr;
	int status = getaddrinfo(address.c_str(), CSTRING(port), &socket.hints, &result);
	if (status != 0) {
		WARNING("Failed to get address info for " << address << ":" << port <<"\nStatus: " << status);
		return false;
	}
	while (result) {
		socket.addr = *((SOCKADDR_IN*)result->ai_addr);
		socket.hints.ai_family = result->ai_family;
		freeaddrinfo(result);
		break; // result = result->ai_next;
	}
	return connect_socket(id);
}

static bool socket_receive(int id) {
	auto& socket = winsock.sockets[id];
	auto data = new iocp_receive_data;
	socket.io.receive.emplace(data);
	// unlike regular non-blocking recv(), WSARecv() will complete asynchronously. this can happen before it returns
	DWORD flags = 0;
	int result = WSARecv(socket.handle, &data->buffer, 1, &data->bytes, &flags, &data->overlapped, nullptr);
	if (result == SOCKET_ERROR) {
		int error = WSAGetLastError();
		switch (error) {
		case WSAECONNRESET:
			socket.sync.disconnect.emplace_and_push(socket_close_status::connection_reset);
			return false;
		case WSAENOTSOCK:
			WS_PRINT_ERROR(error);
			return false;
		case WSA_IO_PENDING:
			return true; // normal error message if the data wasn't received immediately
		default:
			WS_PRINT_ERROR(error);
			return false;
		}
	}
	return true;
}

static bool socket_send(int id, const io_stream& packet) {
	auto& socket = winsock.sockets[id];
	auto data = new iocp_send_data;
	socket.io.send.emplace(data);
	data->buffer = { packet.write_index(), packet.data() };
	// unlike regular non-blocking send(), WSASend() will complete the operation asynchronously,
	// and this might happen before it returns. the data can immediately be discarded on return.
	int result = WSASend(socket.handle, &data->buffer, 1, &data->bytes, 0, &data->overlapped, nullptr);
	if (result == SOCKET_ERROR) {
		int error = WSAGetLastError();
		switch (error) {
		case WSAECONNRESET:
			socket.sync.disconnect.emplace_and_push(socket_close_status::connection_reset);
			return false;
		case WSA_IO_PENDING:
			return true; // normal error message if the data wasn't sent immediately
		default:
			WS_PRINT_ERROR(error);
			return false;
		}
	}
	return true;
}

static bool accept_ex(int id) {
	if (!winsock.AcceptEx) {
		return false;
	}
	auto data = new iocp_accept_data;
	data->accepted_id = open_socket();
	create_socket(data->accepted_id);
	auto& accepted = winsock.sockets[data->accepted_id];
	auto& socket = winsock.sockets[id];
	socket.io.accept.emplace(data);
	const DWORD addr_size = iocp_accept_data::padded_addr_size;
	BOOL status = winsock.AcceptEx(socket.handle, accepted.handle, data->buffer.buf, 0, addr_size, addr_size, &data->bytes, &data->overlapped);
	if (status == FALSE) {
		int error = WSAGetLastError();
		switch (error) {
		case WSA_IO_PENDING:
			return true; // normal error message if a client wasn't accepted immediately
		default:
			WS_PRINT_ERROR(error);
			destroy_socket(data->accepted_id);
			socket.io.accept.erase(data);
			delete data;
			return false;
		}
	}
	return true;
}

DWORD io_port_thread(HANDLE io_port, int thread_num) {
	int log = thread_num + 1;
	while (true) {
		DWORD transferred = 0; // bytes transferred during this operation
		ULONG_PTR completion_key = 0; // pointer to winsock_socket the operation was completed on
		LPOVERLAPPED overlapped = nullptr; // pointer to overlapped structure inside winsock_io_data

		// associate this thread with the completion port as we get the queued completion status
		BOOL status = GetQueuedCompletionStatus(io_port, &transferred, &completion_key, &overlapped, INFINITE);
		if (!status) {
			WS_PRINT_LAST_ERROR_X(log);
			if (!overlapped) {
				continue;
			}
			// if overlapped is not a nullptr, a completion status was dequeued
			// this means transferred, completion_key, and overlapped are valid
			// the socket probably disconnected, but that will be handled below
		}
		// reference for the macro: https://msdn.microsoft.com/en-us/library/aa447688.aspx
		auto data = CONTAINING_RECORD(overlapped, iocp_data<iocp_operation::invalid>, overlapped);
		if (data->operation == iocp_operation::close) {
			INFO_X(log, "Leaving thread");
			return 0;
		}
		int socket_id = (int)completion_key;
		std::lock_guard lock{ *winsock.mutexes[socket_id] };
		auto& socket = winsock.sockets[socket_id];

		if (data->operation == iocp_operation::send) {
			auto send_data = (iocp_send_data*)data;
			if (transferred == 0) {
				socket.sync.disconnect.emplace_and_push(socket_close_status::disconnected_gracefully);
				continue;
			}
			socket.io.send.erase(send_data);
			delete send_data;

		} else if (data->operation == iocp_operation::receive) {
			auto receive_data = (iocp_receive_data*)data;
			if (transferred == 0) {
				socket.sync.disconnect.emplace_and_push(socket_close_status::disconnected_gracefully);
				continue;
			}
			size_t previous_write = socket.receive_packetizer.write_index();
			socket.receive_packetizer.write(receive_data->buffer.buf, transferred);
			// queue the stream events. use the packetizer's buffer
			char* stream_begin = socket.receive_packetizer.data() + previous_write;
			socket.sync.stream.emplace_and_push(stream_begin, transferred, io_stream::construct_by::shallow_copy);
			// parse buffer and queue packet events
			while (true) {
				io_stream packet = socket.receive_packetizer.next();
				if (packet.size() == 0) {
					break;
				}
				socket.sync.packet.emplace_and_push(packet.data(), packet.size_left_to_read(), io_stream::construct_by::shallow_copy);
			}
			socket.io.receive.erase(receive_data);
			delete receive_data;
			socket_receive(socket_id);

		} else if (data->operation == iocp_operation::accept) {
			auto accept_data = (iocp_accept_data*)data;
			get_accept_sockaddrs(*accept_data);
			auto& accepted = winsock.sockets[accept_data->accepted_id];
			int status = update_accept_context(accepted.handle, socket.handle);
			if (status != NO_ERROR) {
				WARNING_X(log, "Failed to update context for accepted socket " << accept_data->accepted_id);
				WS_PRINT_LAST_ERROR_X(log);
				// todo: should the socket be closed here?
			}
			accepted.connected = true;
			socket.sync.accept.emplace_and_push(accept_data->accepted_id);
			socket.io.accept.erase(accept_data);
			delete accept_data;
			increment_socket_accepts(socket_id);
		}
	}
	return 0;
}

void start_network() {
	MESSAGE("Initializing WinSock");
	WORD version = MAKEWORD(2, 2);
	int status = WSAStartup(version, &winsock.wsa_data);
	if (status != 0) {
		CRITICAL("WinSock failed to start. Error: " << status);
		return;
	}
	create_completion_port();
}

void stop_network() {
	destroy_completion_port();
	for (int i = 0; i < (int)winsock.sockets.size(); i++) {
		destroy_socket(i);
	}
	int result = WSACleanup();
	if (result != 0) {
		WARNING("Failed to stop WinSock. Some operations may still be ongoing.");
		print_winsock_error(WSAGetLastError(), __FUNCSIG__, __LINE__, 0);
		return;
	}
	MESSAGE("WinSock has been stopped.");
}

int open_socket() {
	for (int i = 0; i < (int)winsock.sockets.size(); i++) {
		if (!winsock.sockets[i].alive) {
			winsock.sockets[i] = {};
			winsock.sockets[i].alive = true;
			return i;
		}
	}
	int id = (int)winsock.sockets.size();
	winsock.mutexes.emplace_back(std::make_unique<std::mutex>());
	winsock.sockets.emplace_back().alive = true;
	return id;
}

int open_socket(const std::string& address, int port) {
	int id = open_socket();
	if (connect_socket(id, address, port)) {
		socket_receive(id);
	}
	return id;
}

void close_socket(int id) {
	winsock.destroy_queue.push_back(id);
}

void synchronize_socket(int id) {
	std::lock_guard lock{ *winsock.mutexes[id] };
	auto& socket = winsock.sockets[id];
	if (socket.sync.disconnect.size() > 0) {
		socket.sync.disconnect.emit(socket.events.disconnect);
		close_socket(id);
		return;
	}
	if (socket.connected) {
		socket.sync.stream.emit(socket.events.stream);
		socket.sync.packet.emit(socket.events.packet);
		socket.receive_packetizer.clean();
		for (auto& packet : socket.queued_packets) {
			socket_send(id, packet);
		}
	}
	socket.queued_packets.clear();
	if (socket.listening) {
		socket.sync.accept.all([&](int accepted_id) {
			socket.events.accept.emit(accepted_id);
			socket_receive(accepted_id);
		});
	}
}

void synchronise_sockets() {
	for (int i = 0; i < (int)winsock.sockets.size(); i++) {
		if (winsock.sockets[i].alive) {
			synchronize_socket(i);
		}
	}
	winsock.queued_packets.clear();
	for (int destroy_id : winsock.destroy_queue) {
		destroy_socket(destroy_id);
	}
	winsock.destroy_queue.clear();
}

bool bind_socket(int id, const std::string& address, int port) {
	auto& socket = winsock.sockets[id];
	addrinfo* result = nullptr;
	int status = getaddrinfo(address.c_str(), CSTRING(port), &socket.hints, &result);
	if (status != 0) {
		WARNING("Failed to get address info for " << address << ":" << port << "\nStatus: " << status);
		return false;
	}
	while (result) {
		socket.addr = *((SOCKADDR_IN*)result->ai_addr);
		socket.hints.ai_family = result->ai_family;
		freeaddrinfo(result);
		break; // result = result->ai_next;
	}
	create_socket(id);
	status = ::bind(socket.handle, (SOCKADDR*)&socket.addr, socket.addr_size);
	if (status != 0) {
		WS_PRINT_LAST_ERROR();
		return false;
	}
	return true;
}

bool listen_socket(int id) {
	auto& socket = winsock.sockets[id];
	int success = ::listen(socket.handle, SOMAXCONN);
	if (success != 0) {
		WS_PRINT_LAST_ERROR();
		return false;
	}
	socket.listening = true;
	load_extensions(socket.handle);
	return increment_socket_accepts(id);
}

bool increment_socket_accepts(int id) {
	// use completion ports with AcceptEx extension if loaded
	if (accept_ex(id)) {
		return true;
	}
	// use regular accept instead
	auto& socket = winsock.sockets[id];
	SOCKET accepted_handle = ::accept(socket.handle, (SOCKADDR*)&socket.addr, &socket.addr_size);
	if (accepted_handle == SOCKET_ERROR) {
		// if the socket is non-blocking, there probably weren't any connections to be accepted
		if (WSAGetLastError() != WSAEWOULDBLOCK) {
			WS_PRINT_LAST_ERROR();
		}
		return false;
	}
	int accept_id = open_socket();
	winsock.sockets[accept_id].handle = accepted_handle;
	socket.events.accept.emit(accept_id);
	return true;
}

void socket_send(int id, io_stream&& stream) {
	winsock.sockets[id].queued_packets.emplace_back(std::move(stream));
}

void broadcast(io_stream&& stream) {
	auto& packet = winsock.queued_packets.emplace_back(std::move(stream));
	for (auto& socket : winsock.sockets) {
		socket.queued_packets.emplace_back(packet.data(), packet.size(), io_stream::construct_by::shallow_copy);
	}
}

void broadcast(io_stream&& stream, int except_id) {
	auto& packet = winsock.queued_packets.emplace_back(std::move(stream));
	for (int i = 0; i < (int)winsock.sockets.size(); i++) {
		if (i != except_id) {
			winsock.sockets[i].queued_packets.emplace_back(packet.data(), packet.size(), io_stream::construct_by::shallow_copy);
		}
	}
}

socket_events& socket_event(int id) {
	return winsock.sockets[id].events;
}

}

std::ostream& operator<<(std::ostream& out, no::iocp_operation operation) {
	switch (operation) {
	case no::iocp_operation::invalid: return out << "Invalid";
	case no::iocp_operation::send: return out << "Send";
	case no::iocp_operation::receive: return out << "Receive";
	case no::iocp_operation::accept: return out << "Accept";
	case no::iocp_operation::close: return out << "Close";
	default: return out << "Unknown (" << (int)operation << ")";
	}
}

#endif
