#include "windows_sockets.hpp"

#if PLATFORM_WINDOWS

#define WS_PRINT_ERROR(ERR)      winsock->print_error(ERR, __FUNCSIG__, __LINE__, 0)
#define WS_PRINT_ERROR_X(ERR, X) winsock->print_error(ERR, __FUNCSIG__, __LINE__, X)
#define WS_PRINT_LAST_ERROR()    winsock->print_error(WSAGetLastError(), __FUNCSIG__, __LINE__, 0)

namespace no {

DWORD io_port_thread(HANDLE io_port, int thread_num);

static winsock_state* winsock = nullptr;

std::string get_winsock_error_name(int error) {
	switch (error) {
	case WSANOTINITIALISED: return "WSANOTINITIALISED";
	case WSAECONNRESET: return "WSAECONNRESET";
	case WSAEFAULT: return "WSAEFAULT";
	case WSAEINTR: return "WSAEINTR";
	case WSAEINVAL: return "WSAEINVAL";
	case WSAEINPROGRESS: return "WSAEINPROGRESS";
	case WSAEMFILE: return "WSAEMFILE";
	case WSAENETDOWN: return "WSAENETDOWN";
	case WSAENOBUFS: return "WSAENOBUFS";
	case WSAENOTSOCK: return "WSAENOTSOCK";
	case WSAEOPNOTSUPP: return "WSAEOPNOTSUPP";
	case WSAEWOULDBLOCK: return "WSAEWOULDBLOCK";
	case WSAEADDRINUSE: return "WSAEADDRINUSE";
	case WSAEISCONN: return "WSAEISCONN";
	case WSAEDESTADDRREQ: return "WSAEDESTADDRREQ";
	case WSAEAFNOSUPPORT: return "WSAEAFNOSUPPORT";
	case WSAEADDRNOTAVAIL: return "WSAEADDRNOTAVAIL";
	case WSAECONNREFUSED: return "WSAECONNREFUSED";
	default: return std::to_string(error);
	}
}

void start_network() {
	if (winsock) {
		WARNING("WinSock has already been initialized.");
		return;
	}
	MESSAGE("Initializing WinSock");
	winsock = new winsock_state();
	if (winsock->status() != 0) {
		delete winsock;
		winsock = nullptr;
	}
}

void stop_network() {
	if (!winsock) {
		WARNING("WinSock has not started. No need to clean up.");
		return;
	}
	delete winsock;
	winsock = nullptr;
}

iocp_data::iocp_data(iocp_data&& that) {
	std::swap(overlapped, that.overlapped);
	std::swap(bytes, that.bytes);
}

iocp_data& iocp_data::operator=(iocp_data&& that) {
	std::swap(overlapped, that.overlapped);
	std::swap(bytes, that.bytes);
	return *this;
}

iocp_operation iocp_data::operation() const {
	return iocp_operation::invalid;
}

iocp_send_data::iocp_send_data(iocp_send_data&& that) : iocp_data{ std::move(that) }, location{ that.location } {
	std::swap(buffer, that.buffer);
	std::swap(location, that.location);
}

iocp_send_data& iocp_send_data::operator=(iocp_send_data&& that) {
	iocp_data::operator=(std::move(that));
	std::swap(buffer, that.buffer);
	std::swap(location, that.location);
	return *this;
}

iocp_operation iocp_send_data::operation() const {
	return iocp_operation::send;
}

iocp_receive_data::iocp_receive_data(iocp_receive_data&& that) : iocp_data{ std::move(that) }, location{ nullptr } {
	std::swap(data, that.data);
	std::swap(buffer, that.buffer);
	std::swap(location, that.location);
}

iocp_receive_data& iocp_receive_data::operator=(iocp_receive_data&& that) {
	iocp_data::operator=(std::move(that));
	std::swap(data, that.data);
	std::swap(buffer, that.buffer);
	std::swap(location, that.location);
	return *this;
}

iocp_operation iocp_receive_data::operation() const {
	return iocp_operation::receive;
}

iocp_accept_data::iocp_accept_data(iocp_accept_data&& that) : iocp_data{ std::move(that) } {
	std::swap(data, that.data);
	std::swap(buffer, that.buffer);
	std::swap(listener, that.listener);
	std::swap(id, that.id);
}

iocp_accept_data& iocp_accept_data::operator=(iocp_accept_data&& that) {
	iocp_data::operator=(std::move(that));
	std::swap(data, that.data);
	std::swap(buffer, that.buffer);
	std::swap(listener, that.listener);
	std::swap(id, that.id);
	return *this;
}

iocp_operation iocp_accept_data::operation() const {
	return iocp_operation::accept;
}

iocp_operation iocp_close_data::operation() const {
	return iocp_operation::close;
}

winsock_socket::winsock_socket() {
	set_protocol(ip_protocol::tcp);
	set_address_family(address_family::inet4);
}

winsock_socket::winsock_socket(winsock_socket&& that) : winsock_socket{} {
	std::swap(handle, that.handle);
	std::swap(hints, that.hints);
	std::swap(addr, that.addr);
	std::swap(addr_size, that.addr_size);
	std::swap(io, that.io);
	std::swap(receive_packetizer, that.receive_packetizer);
	std::swap(received, that.received);
	std::swap(listening, that.listening);
	std::swap(AcceptEx, that.AcceptEx);
	std::swap(GetAcceptExSockaddrs, that.GetAcceptExSockaddrs);
}

winsock_socket::~winsock_socket() {
	close();
}

winsock_socket& winsock_socket::operator=(winsock_socket&& that) {
	std::swap(handle, that.handle);
	std::swap(hints, that.hints);
	std::swap(addr, that.addr);
	std::swap(addr_size, that.addr_size);
	std::swap(io, that.io);
	std::swap(receive_packetizer, that.receive_packetizer);
	std::swap(received, that.received);
	std::swap(listening, that.listening);
	std::swap(AcceptEx, that.AcceptEx);
	std::swap(GetAcceptExSockaddrs, that.GetAcceptExSockaddrs);
	return *this;
}

bool winsock_socket::open() {
	if (handle != INVALID_SOCKET) {
		return true;
	}
	handle = WSASocketW(hints.ai_family, hints.ai_socktype, hints.ai_protocol, nullptr, 0, WSA_FLAG_OVERLAPPED);
	if (handle == INVALID_SOCKET) {
		WS_PRINT_LAST_ERROR();
		return false;
	}
	CreateIoCompletionPort((HANDLE)handle, winsock->port_handle(), (ULONG_PTR)this, 0);
	return true;
}

bool winsock_socket::close() {
	if (handle == INVALID_SOCKET) {
		return true;
	}
	int success = closesocket(handle);
	if (success == SOCKET_ERROR) {
		WS_PRINT_LAST_ERROR();
		return false;
	}
	handle = INVALID_SOCKET;
	return true;
}

bool winsock_socket::bind() {
	open();
	int success = ::bind(handle, (SOCKADDR*)&addr, addr_size);
	if (success != 0) {
		WS_PRINT_LAST_ERROR();
		return false;
	}
	return true;
}

bool winsock_socket::bind(const std::string& address, int port) {
	addrinfo* result = nullptr;
	int status = getaddrinfo(address.c_str(), CSTRING(port), &hints, &result);
	if (status != 0) {
		WARNING("Failed to get address info for " << address << ":" << port <<"\nStatus: " << status);
		return false;
	}
	// todo: make ipv6 compatible
	while (result) {
		addr = *((SOCKADDR_IN*)result->ai_addr);
		hints.ai_family = result->ai_family;
		freeaddrinfo(result);
		break; // result = result->ai_next;
	}
	return bind();
}

bool winsock_socket::connect() {
	open();
	int success = ::connect(handle, (SOCKADDR*)&addr, addr_size);
	if (success != 0) {
		WS_PRINT_LAST_ERROR();
		close();
		return false;
	}
	return true;
}

bool winsock_socket::connect(const std::string& address, int port) {
	addrinfo* result = nullptr;
	int status = getaddrinfo(address.c_str(), CSTRING(port), &hints, &result);
	if (status != 0) {
		WARNING("Failed to get address info for " << address << ":" << port <<"\nStatus: " << status);
		return false;
	}
	// todo: make ipv6 compatible
	while (result) {
		addr = *((SOCKADDR_IN*)result->ai_addr);
		hints.ai_family = result->ai_family;
		freeaddrinfo(result);
		break; // result = result->ai_next;
	}
	return connect();
}

bool winsock_socket::receive(io_socket& socket) {
	auto data = new iocp_receive_data{};
	io.receive.emplace(data);
	data->location = { &socket };
	// unlike regular non-blocking recv(), WSARecv() will complete asynchronously. this can happen before it returns
	DWORD flags = 0;
	int result = WSARecv(handle, &data->buffer, 1, &data->bytes, &flags, &data->overlapped, nullptr);
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

bool winsock_socket::send(const io_stream& packet, io_socket& socket) {
	auto data = new iocp_send_data{};
	io.send.emplace(data);
	data->buffer = { packet.write_index(), packet.data() };
	data->location = { &socket };

	// unlike regular non-blocking send(), WSASend() will complete the operation asynchronously,
	// and this might happen before it returns. the data can immediately be discarded on return.
	int result = WSASend(handle, &data->buffer, 1, &data->bytes, 0, &data->overlapped, nullptr);
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

bool winsock_socket::set_protocol(ip_protocol protocol) {
	switch (protocol) {
	case ip_protocol::tcp:
		hints.ai_protocol = IPPROTO_TCP;
		hints.ai_socktype = SOCK_STREAM;
		return true;
	case ip_protocol::udp:
		hints.ai_protocol = IPPROTO_UDP;
		hints.ai_socktype = SOCK_DGRAM;
		return true;
	default:
		WARNING("Invalid protocol " << protocol);
		return false;
	}
}

bool winsock_socket::set_address_family(address_family family) {
	switch (family) {
	case address_family::inet4:
		hints.ai_family = AF_INET;
		addr.sin_family = hints.ai_family;
		return true;
	case address_family::inet6:
		hints.ai_family = AF_INET6;
		addr.sin_family = hints.ai_family;
		return true;
	default:
		hints.ai_family = AF_UNSPEC;
		addr.sin_family = hints.ai_family;
		WARNING("Invalid address family " << family);
		return false;
	}
}

bool winsock_socket::listen(listener_socket* listener) {
	int success = ::listen(handle, SOMAXCONN);
	if (success != 0) {
		WS_PRINT_LAST_ERROR();
		return false;
	}
	listening = true;
	load_extensions();
	return accept(listener);
}

bool winsock_socket::accept(listener_socket* listener) {
	// use completion ports with AcceptEx extension if loaded
	if (AcceptEx) {
		auto data = new iocp_accept_data{};
		io.accept.emplace(data);
		data->listener = listener;

		// required to open the socket manually
		data->id = winsock->create_socket();
		winsock_socket& ws_accepted = winsock->socket(data->id);
		ws_accepted.open();

		const DWORD addr_size = iocp_accept_data::padded_addr_size;
		BOOL status = AcceptEx(handle, ws_accepted.handle, data->buffer.buf, 0, addr_size, addr_size, &data->bytes, &data->overlapped);
		if (status == FALSE) {
			int error = WSAGetLastError();
			switch (error) {
			case WSA_IO_PENDING:
				return true; // normal error message if a client wasn't accepted immediately
			default:
				WS_PRINT_ERROR(error);
				winsock->destroy_socket(data->id);
				io.accept.erase(data);
				return false;
			}
		}
		return true;
	}

	// use regular accept instead
	SOCKET accepted_handle = ::accept(handle, (SOCKADDR*)&addr, &addr_size);
	if (accepted_handle == SOCKET_ERROR) {
		int error = WSAGetLastError();
		// if the socket is non-blocking, there probably weren't any connections to be accepted
		if (error == WSAEWOULDBLOCK) {
			return false;
		}
		WS_PRINT_LAST_ERROR();
		return false;
	}

	listener_socket::accept_event accept;
	accept.id = winsock->create_socket();
	winsock->socket(accept.id).handle = accepted_handle;
	listener->events.accept.emit(accept);

	return true;
}

void winsock_socket::get_accept_sockaddrs(iocp_accept_data& data) {
	if (!GetAcceptExSockaddrs) {
		return;
	}
	// todo: at the moment we aren't doing anything with the result here
	DWORD addr_size = iocp_accept_data::padded_addr_size;
	sockaddr* local = nullptr;
	sockaddr* remote = nullptr;
	int local_size = sizeof(local);
	int remote_size = sizeof(remote);
	GetAcceptExSockaddrs(data.buffer.buf, 0, addr_size, addr_size, &local, &local_size, &remote, &remote_size);
}

void winsock_socket::load_extensions() {
	if (!listening) {
		return; // these functions are only available for listening sockets
	}
	// AcceptEx
	DWORD bytes = 0;
	DWORD code = SIO_GET_EXTENSION_FUNCTION_POINTER;
	GUID guid = WSAID_ACCEPTEX;
	WSAIoctl(handle, code, &guid, sizeof(guid), &AcceptEx, sizeof(AcceptEx), &bytes, nullptr, nullptr);
	// GetAcceptExSockaddrs
	bytes = 0;
	guid = WSAID_GETACCEPTEXSOCKADDRS;
	WSAIoctl(handle, code, &guid, sizeof(guid), &GetAcceptExSockaddrs, sizeof(GetAcceptExSockaddrs), &bytes, nullptr, nullptr);
}

winsock_state::winsock_state() {
	WORD version = MAKEWORD(2, 2);
	setup_result = WSAStartup(version, &wsa_data);
	if (setup_result != 0) {
		CRITICAL("WinSock failed to start. Error: " << setup_result);
		return;
	}
	create_completion_port();
}

winsock_state::~winsock_state() {
	destroy_completion_port();
	for (auto& i : sockets) {
		i.second.close();
	}
	int result = WSACleanup();
	if (result != 0) {
		WARNING("Failed to stop WinSock. Some operations may still be ongoing.");
		print_error(WSAGetLastError(), __FUNCSIG__, __LINE__, 0);
		return;
	}
	MESSAGE("WinSock has been stopped.");
}

void winsock_state::create_completion_port() {
	// note: 0 means to use number of CPUs on the system
	// note: it's possible to create more threads than specified below,
	//       but the system will only let max this amount of threads to be run simultaneously.
	const int thread_count = 2;
	io_port = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, thread_count);
	if (!io_port) {
		CRITICAL("Failed to create I/O completion port. Error: " << GetLastError());
		return;
	}
	for (int i = 0; i < thread_count; i++) {
		threads.emplace_back([this, i] {
			io_port_thread(io_port, i);
		});
	}
}

void winsock_state::destroy_completion_port() {
	if (io_port == INVALID_HANDLE_VALUE) {
		return;
	}
	iocp_close_data close_io;
	for (auto& thread : threads) {
		PostQueuedCompletionStatus(io_port, 0, 0, &close_io.overlapped);
	}
	// join all threads - each should receive their own close event
	for (auto& thread : threads) {
		if (thread.joinable()) {
			thread.join();
		}
	}
	threads.clear();
	CloseHandle(io_port);
	io_port = INVALID_HANDLE_VALUE;
}

int winsock_state::create_socket() {
	socket_id_counter++;
	sockets[socket_id_counter] = {};
	return socket_id_counter;
}

bool winsock_state::destroy_socket(int id) {
	if (sockets.find(id) == sockets.end()) {
		return false;
	}
	sockets.erase(id);
	return true;
}

void winsock_state::print_error(int error, const std::string& funcsig, int line, int log) const {
	CRITICAL_X(log, "WSA Error " << get_winsock_error_name(error) << " (" << error << ") on line " << line << " in function " << funcsig);
}

int winsock_state::status() const {
	return setup_result;
}

HANDLE winsock_state::port_handle() const {
	return io_port;
}

winsock_socket& winsock_state::socket(int id) {
	return sockets.find(id)->second;
}

DWORD io_port_thread(HANDLE io_port, int thread_num) {
	thread_num++;
	while (true) {
		DWORD transferred = 0; // bytes transferred during this operation
		ULONG_PTR completion_key = 0; // pointer to winsock_socket the operation was completed on
		LPOVERLAPPED overlapped = nullptr; // pointer to overlapped structure inside winsock_io_data

		// associate this thread with the completion port as we get the queued completion status
		BOOL status = GetQueuedCompletionStatus(io_port, &transferred, &completion_key, &overlapped, INFINITE);
		if (!status) {
			WARNING_X(thread_num, "An error occurred while reading the completion status.\nError: " << GetLastError());
			if (!overlapped) {
				continue;
			}
			// if overlapped is not a nullptr, a completion status was dequeued
			// this means transferred, completion_key, and overlapped are valid
			// the socket probably disconnected, but that will be handled below
		}

		// reference for the macro: https://msdn.microsoft.com/en-us/library/aa447688.aspx
		auto data = CONTAINING_RECORD(overlapped, iocp_data, overlapped);
		if (data->operation() == iocp_operation::close) {
			INFO_X(thread_num, "Leaving thread");
			return 0;
		}
		if (!completion_key) {
			WARNING_X(thread_num, "Invalid completion key for operation " << data->operation());
			continue;
		}
		auto& ws_socket = *(winsock_socket*)completion_key;

		// prevent multiple occurring receives, sends, and accepts from being processed simultaneously
		std::lock_guard lock{ ws_socket.mutex };

		if (data->operation() == iocp_operation::send) {
			iocp_send_data* send_data = (iocp_send_data*)data;
			io_socket* socket = send_data->location.find();
			if (transferred == 0) {
				socket->sync.disconnect.emplace_and_push(socket_close_status::disconnected_gracefully);
				continue;
			}
			ws_socket.io.send.erase(send_data);
			delete send_data;

		} else if (data->operation() == iocp_operation::receive) {
			iocp_receive_data* receive_data = (iocp_receive_data*)data;
			io_socket* socket = receive_data->location.find();
			if (transferred == 0) {
				socket->sync.disconnect.emplace_and_push(socket_close_status::disconnected_gracefully);
				continue;
			}
			size_t previous_write = ws_socket.receive_packetizer.write_index();
			ws_socket.receive_packetizer.write(receive_data->buffer.buf, transferred);

			// queue the stream events. we can use the packetizer's buffer
			char* stream_begin = ws_socket.receive_packetizer.data() + previous_write;
			socket->sync.receive_stream.emplace_and_push(stream_begin, transferred, io_stream::construct_by::shallow_copy);

			// parse the buffer and queue the packet events for every complete packet
			while (true) {
				io_stream packet = ws_socket.receive_packetizer.next();
				if (packet.size() == 0) {
					break;
				}
				socket->sync.receive_packet.emplace_and_push(packet.data(), packet.size_left_to_read(), io_stream::construct_by::shallow_copy);
			}

			ws_socket.io.receive.erase(receive_data);
			delete receive_data;

			// queue another receive
			socket->receive();

		} else if (data->operation() == iocp_operation::accept) {
			iocp_accept_data* accept_data = (iocp_accept_data*)data;
			listener_socket* listener = accept_data->listener;
			ws_socket.get_accept_sockaddrs(*accept_data);

			winsock_socket& ws_accept = winsock->socket(accept_data->id);
			int status = ws_accept.update_accept_context(ws_socket);
			if (status != NO_ERROR) {
				WARNING_X(thread_num, "Failed to update context for accepted socket " << accept_data->id << "(See below)");
				WS_PRINT_LAST_ERROR();
				// todo: should the socket be closed here?
			}

			listener_socket::accept_event accept;
			accept.id = accept_data->id;
			listener->sync.accept.move_and_push(std::move(accept));

			ws_socket.io.accept.erase(accept_data);
			delete accept_data;

			// start accepting a new client
			listener->accept();
		}
	}
	return 0;
}

int winsock_socket::update_accept_context(const winsock_socket& client) {
	return setsockopt(handle, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (char*)&client.handle, sizeof(client.handle));
}

io_socket::io_socket() : location{ this } {

}

io_socket::io_socket(int id, const socket_location& location) : abstract_socket(id), location(location) {

}

io_socket::io_socket(io_socket&& that) : abstract_socket{ std::move(that) }, location{ this } {
	std::swap(events, that.events);
	std::swap(sync, that.sync);
}

io_socket& io_socket::operator=(io_socket&& that) {
	abstract_socket::operator=(std::move(that));
	std::swap(events, that.events);
	std::swap(sync, that.sync);
	return *this;
}

void io_socket::send(io_stream&& packet) {
	queued_packets.emplace_back(std::move(packet));
}

bool io_socket::receive() {
	return winsock->socket(id()).receive(*this);
}

bool io_socket::connect() {
	if (id() == -1) {
		socket_id = winsock->create_socket();
	}
	auto& socket = winsock->socket(id());
	if (!socket.connect()) {
		return false;
	}
	receive();
	return true;
}

bool io_socket::connect(const std::string& address, int port) {
	if (id() == -1) {
		socket_id = winsock->create_socket();
	}
	if (!winsock->socket(id()).connect(address, port)) {
		return false;
	}
	receive();
	return true;
}

void io_socket::synchronise() {
	winsock_socket& ws_socket = winsock->socket(id());

	// make sure the thread doesn't interrupt the packetizer. we need its buffer intact
	std::lock_guard lock{ ws_socket.mutex };

	if (sync.disconnect.size() > 0) {
		sync.disconnect.emit(events.disconnect);
		ws_socket.close();
		socket_id = -1;
		return;
	}

	for (auto& packet : queued_packets) {
		send_synced(packet);
	}
	queued_packets.clear();

	sync.receive_stream.emit(events.receive_stream);
	sync.receive_packet.emit(events.receive_packet);

	ws_socket.receive_packetizer.clean();
}

bool io_socket::send_synced(const io_stream& packet) {
	return winsock->socket(id()).send(packet, *this);
}

listener_socket::listener_socket(listener_socket&& that) : abstract_socket{ std::move(that) } {
	std::swap(events, that.events);
	std::swap(sync, that.sync);
}

listener_socket& listener_socket::operator=(listener_socket&& that) {
	abstract_socket::operator=(std::move(that));
	std::swap(events, that.events);
	std::swap(sync, that.sync);
	return *this;
}

bool listener_socket::bind() {
	if (id() == -1) {
		socket_id = winsock->create_socket();
	}
	return winsock->socket(id()).bind();
}

bool listener_socket::bind(const std::string& address, int port) {
	if (id() == -1) {
		socket_id = winsock->create_socket();
	}
	return winsock->socket(id()).bind(address, port);
}

bool listener_socket::listen() {
	return winsock->socket(id()).listen(this);
}

bool listener_socket::accept() {
	return winsock->socket(id()).accept(this);
}

void listener_socket::synchronise() {
	winsock_socket* ws_socket = &winsock->socket(id());
	std::lock_guard lock{ ws_socket->mutex };
	sync.accept.emit(events.accept);
}

abstract_socket::abstract_socket(int id) : socket_id(id) {

}

abstract_socket::abstract_socket(abstract_socket&& that) {
	std::swap(socket_id, that.socket_id);
}

abstract_socket& abstract_socket::operator=(abstract_socket&& that) {
	std::swap(socket_id, that.socket_id);
	return *this;
}

bool abstract_socket::disconnect() {
	if (socket_id == -1) {
		return false;
	}
	winsock_socket* ws_socket = &winsock->socket(socket_id);
	bool success = winsock->destroy_socket(socket_id);
	if (success) {
		socket_id = -1;
	} else {
		WARNING("Failed to destroy WinSock socket " << socket_id);
	}
	return true;
}

bool abstract_socket::set_protocol(ip_protocol protocol) {
	return winsock->socket(socket_id).set_protocol(protocol);
}

bool abstract_socket::set_address_family(address_family family) {
	return winsock->socket(socket_id).set_address_family(family);
}

int abstract_socket::id() const {
	return socket_id;
}

}

std::ostream& operator<<(std::ostream& out, no::iocp_operation operation) {
	switch (operation) {
	case no::iocp_operation::invalid: return out << "Invalid";
	case no::iocp_operation::send: return out << "Send";
	case no::iocp_operation::receive: return out << "Receive";
	case no::iocp_operation::accept: return out << "Accept";
	case no::iocp_operation::connect: return out << "Connect";
	case no::iocp_operation::close: return out << "Close";
	default: return out << "Unknown (" << (int)operation << ")";
	}
}

std::ostream& operator<<(std::ostream& out, no::ip_protocol protocol) {
	switch (protocol) {
	case no::ip_protocol::tcp: return out << "TCP";
	case no::ip_protocol::udp: return out << "UDP";
	default: return out << "Unknown (" << (int)protocol << ")";
	}
}

#endif
