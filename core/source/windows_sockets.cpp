#include "windows_sockets.hpp"

#if PLATFORM_WINDOWS

#define WS_PRINT_ERROR(ERR)      winsock->print_error(ERR, __FUNCSIG__, __LINE__, 0)
#define WS_PRINT_ERROR_X(ERR, X) winsock->print_error(ERR, __FUNCSIG__, __LINE__, X)
#define WS_PRINT_LAST_ERROR()    winsock->print_error(WSAGetLastError(), __FUNCSIG__, __LINE__, 0)

namespace no {

// inet_addr first, then gethostbyname if failed
// with udp: SO_MAX_MSG_SIZE 

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
	if (winsock->setup_result != 0) {
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

winsock_socket::winsock_socket() {
	reset();
}

winsock_socket::winsock_socket(winsock_socket&& that) {
	handle = that.handle;
	hints = that.hints;
	addr = that.addr;
	addr_size = that.addr_size;
	io = std::move(that.io);
	receive_packetizer = std::move(that.receive_packetizer);
	received = that.received;
	is_listening = that.is_listening;
	AcceptEx = that.AcceptEx;
	GetAcceptExSockaddrs = that.GetAcceptExSockaddrs;
	that.reset();
}

winsock_socket::~winsock_socket() {
	close();
}

winsock_socket& winsock_socket::operator=(winsock_socket&& that) {
	handle = that.handle;
	hints = that.hints;
	addr = that.addr;
	addr_size = that.addr_size;
	io = std::move(that.io);
	receive_packetizer = std::move(that.receive_packetizer);
	received = that.received;
	is_listening = that.is_listening;
	AcceptEx = that.AcceptEx;
	GetAcceptExSockaddrs = that.GetAcceptExSockaddrs;
	that.reset();
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
	// associate the winsock i/o completion port with the socket handle
	// the 'this' pointer is passed as the completion key
	CreateIoCompletionPort((HANDLE)handle, winsock->io_port, (ULONG_PTR)this, 0);
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
		WARNING("Invalid protocol " << (int)protocol);
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
		WARNING("Invalid address family " << (int)family);
		return false;
	}
}

bool winsock_socket::listen(listener_socket* listener) {
	int success = ::listen(handle, SOMAXCONN);
	if (success != 0) {
		WS_PRINT_LAST_ERROR();
		return false;
	}
	is_listening = true;
	load_extensions();
	return accept(listener);
}

bool winsock_socket::accept(listener_socket* listener) {
	// use completion ports with AcceptEx extension if loaded
	if (AcceptEx) {
		winsock_io_accept_data* data = io.accept.create();
		if (!data) {
			return false; // none were available
		}
		data->listener = listener;

		// required to open the socket manually
		data->id = winsock->create_socket();
		winsock_socket* ws_accepted = &winsock->sockets[data->id];
		ws_accepted->open();

		const DWORD addr_size = IO_ACCEPT_ADDR_SIZE_PADDED;
		BOOL status = AcceptEx(handle, ws_accepted->handle, data->buffer.buf, 0, addr_size, addr_size, &data->bytes, &data->overlapped);
		if (status == FALSE) {
			int error = WSAGetLastError();
			switch (error) {
			case WSA_IO_PENDING:
				return true; // normal error message if a client wasn't accepted immediately
			default:
				WS_PRINT_ERROR(error);
				winsock->destroy_socket(data->id);
				io.accept.destroy(data);
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

	listener_socket::accept_message accept;
	accept.id = winsock->create_socket();
	winsock->sockets[accept.id].handle = accepted_handle;
	listener->events.accept.emit(accept);

	return true;
}

void winsock_socket::load_extensions() {
	if (!is_listening) {
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

void winsock_socket::reset() {
	ZeroMemory(&hints, sizeof(hints));
	ZeroMemory(&addr, sizeof(addr));
	addr_size = sizeof(addr);
	handle = INVALID_SOCKET;
	io = {};
	receive_packetizer = {};
	received = { 0, nullptr };
	is_listening = false;
	set_protocol(ip_protocol::tcp);
	set_address_family(address_family::inet4);
	AcceptEx = nullptr;
	GetAcceptExSockaddrs = nullptr;
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
	// how many threads do we want to use for the i/o operations?
	const int thread_count = 2;

	// we want the handle for a new completion port, so we pass in a nullptr as the second argument
	// we also have nothing to associate with the new port yet, so we pass an invalid handle as the first argument
	// thread_count = 0 means to use number of CPUs on the system
	io_port = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, thread_count);
	if (io_port == nullptr) {
		DWORD error = GetLastError();
		CRITICAL("Failed to create I/O completion port. Error: " << error);
		return;
	}

	// now that we have io_port, we can now start associating socket handles with the completion port object

	// create the worker threads that will process the I/O completion statuses
	// we need them to service the completion port when status events are posted to it
	// it is possible to create more worker threads than specified in thread_count,
	// but the system will only let thread_count amount of threads to be run at the same time.
	// this doesn't guarantee that the system runs exactly thread_count threads simultanously
	// it might be desirable to have more than thread_count threads to let a running thread block for a while
	// while the idle one takes over
	for (int i = 0; i < thread_count; i++) {
		threads.emplace_back(std::thread([this, i]() {
			io_port_thread(io_port, i);
		}));
	}
}

void winsock_state::destroy_completion_port() {
	if (io_port == INVALID_HANDLE_VALUE) {
		return;
	}
	// post a close event to all the threads.
	winsock_io_close_data close_io;
	for (auto& i : threads) {
		PostQueuedCompletionStatus(io_port, 0, 0, &close_io.overlapped);
	}
	// join all threads - each should receive their own close event
	for (auto& i : threads) {
		if (i.joinable()) {
			i.join();
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

DWORD io_port_thread(HANDLE io_port, int thread_num) {
	thread_num++;
	while (true) {
		//MESSAGE_X(thread_num, "Getting queued completion status");

		// how many bytes were transferred during this operation
		DWORD transferred = 0;

		// pointer to the winsock_socket that the operation was completed on
		ULONG_PTR completion_key = 0;

		// pointer to the overlapped structure inside the winsock_io_data structure
		LPOVERLAPPED overlapped = nullptr;

		// associate this thread with the completion port as we get the queued completion status
		BOOL status = GetQueuedCompletionStatus(io_port, &transferred, &completion_key, &overlapped, INFINITE);

		//MESSAGE_X(thread_num, "Status: " << status << ". Key: " << completion_key  << ". Bytes transferred: " << transferred);

		if (status == FALSE) {
			WARNING_X(thread_num, "An error occurred while reading the completion status.\nError: " << GetLastError());
			if (!overlapped) {
				continue;
			}
			// if overlapped is not a nullptr, it means we dequeued a completion status
			// we may then use transferred, completion_key and overlapped for more information
			// the socket has most likely disconnected if we get here
			INFO_X(thread_num, "We have the overlapped structure. This means a status event was dequeued.\nA socket has likely disconnected.");
		}

		// retrieve the full I/O data structure
		// reference for the macro: https://msdn.microsoft.com/en-us/library/aa447688.aspx
		winsock_io_abstract_data* data = CONTAINING_RECORD(overlapped, winsock_io_abstract_data, overlapped);
		if (data->operation() == io_completion_port_operation::close) {
			INFO_X(thread_num, "Leaving thread");
			return 0;
		}
		if (!completion_key) {
			WARNING_X(thread_num, "Invalid completion key for operation " << data->operation());
			continue;
		}
		winsock_socket* ws_socket = (winsock_socket*)completion_key;

		// prevent multiple occurring receives, sends and accepts happening at once
		std::lock_guard<std::mutex> lock(ws_socket->mutex);

		if (data->operation() == io_completion_port_operation::send) {
			winsock_io_send_data* send_data = (winsock_io_send_data*)data;
			io_socket* socket = send_data->location.get();
			if (transferred == 0) {
				MESSAGE_X(thread_num, "Socket disconnected.");
				socket->sync.disconnect.emplace_and_push(socket_close_status::disconnected_gracefully);
				continue;
			}
			io_socket::send_message send_message;
			socket->sync.send.move_and_push(std::move(send_message));
			ws_socket->io.send.destroy(send_data);

		} else if (data->operation() == io_completion_port_operation::receive) {
			winsock_io_receive_data* receive_data = (winsock_io_receive_data*)data;
			io_socket* socket = receive_data->location.get();
			if (transferred == 0) {
				MESSAGE_X(thread_num, "Socket disconnected.");
				socket->sync.disconnect.emplace_and_push(socket_close_status::disconnected_gracefully);
				continue;
			}
			char* packetizer_previous_write = ws_socket->receive_packetizer.at_write();
			ws_socket->receive_packetizer.write(receive_data->buffer.buf, transferred);

			MESSAGE_X(thread_num, "Received " << transferred << " bytes");

			// queue the stream events. we can use the packetizer's buffer
			io_socket::receive_stream_message stream_message;
			stream_message.packet = { packetizer_previous_write, transferred, io_stream::construct_by::shallow_copy };
			socket->sync.receive_stream.move_and_push(std::move(stream_message));

			// parse the buffer and queue the packet events for every complete packet
			while (true) {
				io_stream packet = ws_socket->receive_packetizer.next();
				if (packet.size() == 0) {
					break;
				}
				io_socket::receive_packet_message packet_message;
				packet_message.packet = { packet.data(), packet.size_left_to_read(), io_stream::construct_by::shallow_copy };
				socket->sync.receive_packet.move_and_push(std::move(packet_message));
			}

			ws_socket->io.receive.destroy(receive_data);

			// queue another receive
			socket->receive();

		} else if (data->operation() == io_completion_port_operation::accept) {
			winsock_io_accept_data* accept_data = (winsock_io_accept_data*)data;
			listener_socket* listener = accept_data->listener;

			// get the sockaddrs if we have loaded the required extension
			if (ws_socket->GetAcceptExSockaddrs) {
				// todo: at the moment we aren't doing anything with the result here
				DWORD addr_size = IO_ACCEPT_ADDR_SIZE_PADDED;
				sockaddr* local = nullptr;
				sockaddr* remote = nullptr;
				int local_size = sizeof(local);
				int remote_size = sizeof(remote);
				ws_socket->GetAcceptExSockaddrs(accept_data->buffer.buf, 0, addr_size, addr_size, &local, &local_size, &remote, &remote_size);
			}

			winsock_socket* ws_accept = &winsock->sockets[accept_data->id];
			SOCKET listen_socket = ws_socket->handle;
			int opt_status = setsockopt(ws_accept->handle, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (char*)&listen_socket, sizeof(listen_socket));
			if (opt_status != NO_ERROR) {
				WARNING_X(thread_num, "Failed to update context for accepted socket " << accept_data->id << "(See below)");
				WS_PRINT_LAST_ERROR();
				// todo: should the socket be closed here?
			}

			listener_socket::accept_message accept;
			accept.id = accept_data->id;
			listener->sync.accept.move_and_push(std::move(accept));

			ws_socket->io.accept.destroy(accept_data);

			// start accepting a new client
			listener->accept();
		}
	}
	return 0;
}

io_socket::io_socket() {
	location.socket = this;
}

io_socket::io_socket(io_socket&& that) : events(std::move(that.events)), sync(std::move(that.sync)) {
	id = that.id;
	location = that.location;
	location.socket = this;
}

io_socket& io_socket::operator=(io_socket&& that) {
	id = that.id;
	location = that.location;
	location.socket = this;
	events = std::move(that.events);
	sync = std::move(that.sync);
	return *this;
}

bool io_socket::send(const io_stream& packet) {
	winsock_socket* ws_socket = &winsock->sockets[id];
	winsock_io_send_data* data = ws_socket->io.send.create();
	if (!data) {
		return false; // none were available
	}
	// set the buffer to be sent
	data->buffer.buf = packet.data();
	data->buffer.len = packet.write_index();
	// the completion event needs our pointer
	data->location = location;

	// unlike regular non-blocking send(), the WSASend() function will fully complete the operation asynchronously
	//
	// this might happen before the function returns
	// we are informed with a completion port status event either way when it is done
	// however, it must be noted that we never know when the client actually receives the data
	// all we know is that the data has been transferred further down in the network stack, and no longer our business
	//
	/// note: the comments above assume the network is functioning, and that there are no resource limitations
	///       in rare cases, partial sends may occur. we get an error message in the completion port event if so
	int result = WSASend(ws_socket->handle, &data->buffer, 1, &data->bytes, 0, &data->overlapped, nullptr);
	if (result == SOCKET_ERROR) {
		int error = WSAGetLastError();
		switch (error) {
		case WSAECONNRESET:
			sync.disconnect.emplace_and_push(socket_close_status::connection_reset);
			return false;
		case WSA_IO_PENDING:
			return true; // normal error message if the data wasn't sent immediately
		default:
			WS_PRINT_ERROR(error);
			return false;
		}
	}
	return true; // successfully started to send data
}

bool io_socket::send_async(const io_stream& packet) {
	if (begin_async()) {
		bool result = send(packet);
		end_async();
		return result;
	}
	return false;
}

bool io_socket::receive() {
	winsock_socket* ws_socket = &winsock->sockets[id];
	winsock_io_receive_data* data = ws_socket->io.receive.create();
	if (!data) {
		return false; // none were available
	}

	// the completion event needs our pointer
	data->location = location;

	// unlike regular non-blocking recv(), the WSARecv() function will fully complete the operation asynchronously
	// this might happen before the function returns
	// we will be informed with a completion port status event either way when it is done
	DWORD flags = 0;
	int result = WSARecv(ws_socket->handle, &data->buffer, 1, &data->bytes, &flags, &data->overlapped, nullptr);
	if (result == SOCKET_ERROR) {
		int error = WSAGetLastError();
		switch (error) {
		case WSAECONNRESET:
			sync.disconnect.emplace_and_push(socket_close_status::connection_reset);
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
	return true; // successfully started to receive data
}

bool io_socket::connect() {
	if (id == -1) {
		id = winsock->create_socket();
	}
	winsock_socket* ws_socket = &winsock->sockets[id];
	ws_socket->open();
	int success = ::connect(ws_socket->handle, (SOCKADDR*)&ws_socket->addr, ws_socket->addr_size);
	if (success != 0) {
		WS_PRINT_LAST_ERROR();
		ws_socket->close();
		return false;
	}
	receive();
	return true;
}

bool io_socket::connect(const std::string& address, int port) {
	if (id == -1) {
		id = winsock->create_socket();
	}
	winsock_socket* ws_socket = &winsock->sockets[id];
	addrinfo* result = nullptr;
	int status = getaddrinfo(address.c_str(), CSTRING(port), &ws_socket->hints, &result);
	if (status != 0) {
		WARNING("Failed to get address info for " << address << ":" << port <<"\nStatus: " << status);
		return false;
	}
	// todo: make ipv6 compatible
	while (result) {
		ws_socket->addr = *((SOCKADDR_IN*)result->ai_addr);
		ws_socket->hints.ai_family = result->ai_family;
		freeaddrinfo(result);
		break; // result = result->ai_next;
	}
	return connect();
}

void io_socket::synchronise() {
	winsock_socket* ws_socket = &winsock->sockets[id];

	// make sure the thread doesn't interrupt the packetizer. we need its buffer intact
	std::lock_guard lock(ws_socket->mutex);

	if (sync.disconnect.size() > 0) {
		sync.disconnect.emit(events.disconnect);
		ws_socket->close();
		id = -1;
		return;
	}

	sync.receive_packet.emit(events.receive_packet);
	sync.receive_stream.emit(events.receive_stream);
	sync.send.emit(events.send);

	ws_socket->receive_packetizer.clean();
}

bool listener_socket::bind() {
	if (id == -1) {
		id = winsock->create_socket();
	}
	winsock_socket* ws_socket = &winsock->sockets[id];
	ws_socket->open();
	int success = ::bind(ws_socket->handle, (SOCKADDR*)&ws_socket->addr, ws_socket->addr_size);
	if (success != 0) {
		WS_PRINT_LAST_ERROR();
		return false;
	}
	return true;
}

bool listener_socket::bind(const std::string& address, int port) {
	if (id == -1) {
		id = winsock->create_socket();
	}
	winsock_socket* ws_socket = &winsock->sockets[id];
	addrinfo* result = nullptr;
	int status = getaddrinfo(address.c_str(), CSTRING(port), &ws_socket->hints, &result);
	if (status != 0) {
		WARNING("Failed to get address info for " << address << ":" << port <<"\nStatus: " << status);
		return false;
	}
	// todo: make ipv6 compatible
	while (result) {
		ws_socket->addr = *((SOCKADDR_IN*)result->ai_addr);
		ws_socket->hints.ai_family = result->ai_family;
		freeaddrinfo(result);
		break; // result = result->ai_next;
	}
	return bind();
}

bool listener_socket::listen() {
	return winsock->sockets[id].listen(this);
}

bool listener_socket::accept() {
	return winsock->sockets[id].accept(this);
}

void listener_socket::synchronise() {
	winsock_socket* ws_socket = &winsock->sockets[id];
	std::lock_guard<std::mutex> lock(ws_socket->mutex);
	sync.accept.emit(events.accept);
}

bool abstract_socket::set_protocol(ip_protocol protocol) {
	return winsock->sockets[id].set_protocol(protocol);
}

bool abstract_socket::set_address_family(address_family family) {
	return winsock->sockets[id].set_address_family(family);
}

bool abstract_socket::disconnect() {
	if (id == -1) {
		return false;
	}
	winsock_socket* ws_socket = &winsock->sockets[id];
	bool success = winsock->destroy_socket(id);
	if (success) {
		id = -1;
	} else {
		WARNING("Failed to destroy WinSock socket " << id);
	}
	return true;
}

bool abstract_socket::begin_async() {
	// We must check if we still are active, as it might not have been synchronised to the caller.
	if (id == -1) {
		return false;
	}
	// MUST FIX
	// todo: there is technically still a possibility for id to become -1 here
	winsock->sockets[id].mutex.lock();
	return true;
}

void abstract_socket::end_async() {
	winsock->sockets[id].mutex.unlock();
}

}

std::ostream& operator<<(std::ostream& out, no::io_completion_port_operation operation) {
	switch (operation) {
	case no::io_completion_port_operation::invalid: return out << "Invalid";
	case no::io_completion_port_operation::send: return out << "Send";
	case no::io_completion_port_operation::receive: return out << "Receive";
	case no::io_completion_port_operation::accept: return out << "Accept";
	case no::io_completion_port_operation::connect: return out << "Connect";
	case no::io_completion_port_operation::close: return out << "Close";
	default: return out << "Unknown";
	}
}

#endif
