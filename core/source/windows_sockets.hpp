#pragma once

#include <winsock2.h>
#include <ws2tcpip.h>
#include <MSWSock.h>

#include "platform.hpp"
#include "network.hpp"
#include "debug.hpp"
#include "io.hpp"

#include <mutex>

#define MAX_IO_SEND_PER_SOCKET     128
#define MAX_IO_RECEIVE_PER_SOCKET  4
#define MAX_IO_ACCEPT_PER_SOCKET   4

// 256 KiB
#define IO_RECEIVE_BUFFER_SIZE     262144

// the buffer size for the local and remote address must be 16 bytes more than the
// size of the sockaddr structure because the addresses are written in an internal format.
// https://msdn.microsoft.com/en-us/library/windows/desktop/ms737524(v=vs.85).aspx
#define IO_ACCEPT_ADDR_SIZE_PADDED  (sizeof(SOCKADDR_IN) + 16)
#define IO_ACCEPT_BUFFER_SIZE       (IO_ACCEPT_ADDR_SIZE_PADDED * 2)

#if PLATFORM_WINDOWS

namespace no {

enum class io_completion_port_operation { invalid, send, receive, accept, connect, close };

class winsock_io_abstract_data {
public:

	WSAOVERLAPPED overlapped;
	DWORD bytes = 0;

	winsock_io_abstract_data() {
		SecureZeroMemory((PVOID)&overlapped, sizeof(WSAOVERLAPPED));
	}

	virtual ~winsock_io_abstract_data() = default;

	virtual io_completion_port_operation operation() const {
		return io_completion_port_operation::invalid;
	}

};

class winsock_io_send_data : public winsock_io_abstract_data {
public:

	WSABUF buffer = { 0, nullptr };

	socket_location location;

	io_completion_port_operation operation() const {
		return io_completion_port_operation::send;
	}

	winsock_io_send_data() = default;
	winsock_io_send_data(const winsock_io_send_data&) = delete;

	winsock_io_send_data& operator=(const winsock_io_send_data&) = delete;

	winsock_io_send_data& operator=(winsock_io_send_data&& io) {
		buffer = io.buffer;
		location = io.location;
		bytes = io.bytes;
		overlapped = io.overlapped;
		io.buffer = { 0, nullptr };
		io.location = {};
		io.bytes = 0;
		SecureZeroMemory((PVOID)&io.overlapped, sizeof(WSAOVERLAPPED));
		return *this;
	}

	~winsock_io_send_data() {
		// the buffer is allocated in an abstract_packet, which doesn't delete its buffer
		// that task is delegated to this destructor, instead of requiring a manual check
		delete[] buffer.buf;
	}

};

class winsock_io_receive_data : public winsock_io_abstract_data {
public:

	char* data = nullptr;

	WSABUF buffer = { 0, nullptr };

	socket_location location;

	io_completion_port_operation operation() const {
		return io_completion_port_operation::receive;
	}

	winsock_io_receive_data() {
		data = new char[IO_RECEIVE_BUFFER_SIZE];
		buffer = { IO_RECEIVE_BUFFER_SIZE, data };
	}

	winsock_io_receive_data(const winsock_io_receive_data&) = delete;
	winsock_io_receive_data& operator=(const winsock_io_receive_data&) = delete;

	winsock_io_receive_data& operator=(winsock_io_receive_data&& that) {
		data = that.data;
		buffer = that.buffer;
		location = that.location;
		that.data = nullptr;
		that.buffer = { 0, nullptr };
		that.location = {};
		return *this;
	}

	winsock_io_receive_data(winsock_io_receive_data&& that) {
		data = that.data;
		buffer = that.buffer;
		location = that.location;
		that.data = nullptr;
		that.buffer = { 0, nullptr };
		that.location = {};
	}

	~winsock_io_receive_data() {
		delete[] data;
	}

};

class winsock_io_accept_data : public winsock_io_abstract_data {
public:

	char data[IO_ACCEPT_BUFFER_SIZE];

	WSABUF buffer = { 0, nullptr };

	listener_socket* listener = nullptr;
	int id = -1;

	io_completion_port_operation operation() const {
		return io_completion_port_operation::accept;
	}

	winsock_io_accept_data() {
		buffer = { IO_ACCEPT_BUFFER_SIZE, data };
	}

	winsock_io_accept_data(const winsock_io_accept_data& that) {
		memcpy(data, that.data, IO_ACCEPT_BUFFER_SIZE);
		buffer = { IO_ACCEPT_BUFFER_SIZE, data };
		overlapped = that.overlapped;
		bytes = that.bytes;
	}

	winsock_io_accept_data& operator=(const winsock_io_accept_data& that) {
		memcpy(data, that.data, IO_ACCEPT_BUFFER_SIZE);
		buffer = { IO_ACCEPT_BUFFER_SIZE, data };
		overlapped = that.overlapped;
		bytes = that.bytes;
		return *this;
	}

};

class winsock_io_close_data : public winsock_io_abstract_data {
public:

	io_completion_port_operation operation() const {
		return io_completion_port_operation::close;
	}

};

// todo: this class should really be separated into two
//       it is wasteful to store all types of i/o data when either socket type only uses half
class winsock_socket {
public:

	SOCKET handle = INVALID_SOCKET;
	addrinfo hints;
	SOCKADDR_IN addr;
	int addr_size = sizeof(addr);
	std::mutex mutex;
	packetizer receive_packetizer;

	// contains all the i/o data that are currently active for this socket
	struct {
		limited_queue<winsock_io_send_data, MAX_IO_SEND_PER_SOCKET> send;
		limited_queue<winsock_io_receive_data, MAX_IO_RECEIVE_PER_SOCKET> receive;
		limited_queue<winsock_io_accept_data, MAX_IO_ACCEPT_PER_SOCKET> accept;
	} io;

	// stores received buffer until we have recognised a packet
	WSABUF received = { 0, nullptr };

	bool is_listening = false;

	winsock_socket();

	winsock_socket(const winsock_socket& socket) = delete;
	winsock_socket& operator=(const winsock_socket& socket) = delete;

	winsock_socket(winsock_socket&&);
	winsock_socket& operator=(winsock_socket&&);

	~winsock_socket();

	bool open();
	bool close();

	bool set_protocol(ip_protocol protocol);
	bool set_address_family(address_family family);

	bool listen(listener_socket* socket);
	bool accept(listener_socket* socket);

	// load AcceptEx and GetAcceptExSockaddrs
	void load_extensions();

	// if this is a listening socket with async i/o mode, we'll use these extensions
	// otherwise, these are always nullpointer
	LPFN_ACCEPTEX AcceptEx = nullptr;
	LPFN_GETACCEPTEXSOCKADDRS GetAcceptExSockaddrs = nullptr;

private:

	void reset();

};

class winsock_state {
public:

	// worker threads to process the I/O completion port events.
	std::vector<std::thread> threads;

	WSADATA wsa_data;
	HANDLE io_port = INVALID_HANDLE_VALUE;

	// stores the status of WSAStartup()
	// generally it will always be 0, because start_network() will make sure to delete bad winsock states
	int setup_result = -1;

	// A simple counter to avoid duplicate socket keys for our unordered map below.
	int socket_id_counter = 0;

	// todo: this shouldn't use a dynamic container 
	//       a socket can technically change address here while an event has already been posted
	//       that will result in a crash or undefined behaviour
	std::unordered_map<int, winsock_socket> sockets;

	winsock_state();
	~winsock_state();

	int create_socket();
	bool destroy_socket(int id);

	void create_completion_port();
	void destroy_completion_port();

	void print_error(int error, const std::string& funcsig, int line, int log) const;

};

}

std::ostream& operator<<(std::ostream& out, no::io_completion_port_operation operation);

#endif
