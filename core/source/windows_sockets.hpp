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

	winsock_io_abstract_data();
	winsock_io_abstract_data(const winsock_io_abstract_data&) = delete;
	winsock_io_abstract_data(winsock_io_abstract_data&&);

	virtual ~winsock_io_abstract_data() = default;

	winsock_io_abstract_data& operator=(const winsock_io_abstract_data&) = delete;
	winsock_io_abstract_data& operator=(winsock_io_abstract_data&&);

	virtual io_completion_port_operation operation() const;

};

class winsock_io_send_data : public winsock_io_abstract_data {
public:

	WSABUF buffer = { 0, nullptr };
	socket_location location;

	winsock_io_send_data();
	winsock_io_send_data(const winsock_io_send_data&) = delete;
	winsock_io_send_data(winsock_io_send_data&&);

	winsock_io_send_data& operator=(const winsock_io_send_data&) = delete;
	winsock_io_send_data& operator=(winsock_io_send_data&&);

	io_completion_port_operation operation() const override;

};

class winsock_io_receive_data : public winsock_io_abstract_data {
public:

	char* data = nullptr;
	WSABUF buffer = { 0, nullptr };
	socket_location location;

	winsock_io_receive_data();
	winsock_io_receive_data(const winsock_io_receive_data&) = delete;
	winsock_io_receive_data(winsock_io_receive_data&&);

	~winsock_io_receive_data() override;

	winsock_io_receive_data& operator=(const winsock_io_receive_data&) = delete;
	winsock_io_receive_data& operator=(winsock_io_receive_data&&);

	io_completion_port_operation operation() const override;

};

class winsock_io_accept_data : public winsock_io_abstract_data {
public:

	char data[IO_ACCEPT_BUFFER_SIZE];
	WSABUF buffer = { 0, nullptr };
	listener_socket* listener = nullptr;
	int id = -1;

	winsock_io_accept_data();
	winsock_io_accept_data(const winsock_io_accept_data&) = delete;
	winsock_io_accept_data(winsock_io_accept_data&&);

	winsock_io_accept_data& operator=(const winsock_io_accept_data&) = delete;
	winsock_io_accept_data& operator=(winsock_io_accept_data&&);

	io_completion_port_operation operation() const override;

};

class winsock_io_close_data : public winsock_io_abstract_data {
public:

	io_completion_port_operation operation() const override;

};

class winsock_socket {
public:

	packetizer receive_packetizer;
	std::mutex mutex;

	struct {
		limited_queue<winsock_io_send_data, MAX_IO_SEND_PER_SOCKET> send;
		limited_queue<winsock_io_receive_data, MAX_IO_RECEIVE_PER_SOCKET> receive;
		limited_queue<winsock_io_accept_data, MAX_IO_ACCEPT_PER_SOCKET> accept;
	} io;

	winsock_socket();
	winsock_socket(const winsock_socket&) = delete;
	winsock_socket(winsock_socket&&);

	~winsock_socket();

	winsock_socket& operator=(const winsock_socket&) = delete;
	winsock_socket& operator=(winsock_socket&&);

	bool open();
	bool close();

	bool bind();
	bool bind(const std::string& address, int port);
	bool connect();
	bool connect(const std::string& address, int port);
	int update_accept_context(const winsock_socket& client);
	bool receive(io_socket& socket);
	bool send(const io_stream& packet, io_socket& socket);

	bool set_protocol(ip_protocol protocol);
	bool set_address_family(address_family family);

	bool listen(listener_socket* socket);
	bool accept(listener_socket* socket);
	void get_accept_sockaddrs(winsock_io_accept_data& data);
	void load_extensions();

private:

	// stores received buffer until a packet is recognized
	WSABUF received = { 0, nullptr };

	SOCKET handle = INVALID_SOCKET;
	addrinfo hints = {};
	SOCKADDR_IN addr = {};
	int addr_size = sizeof(addr);
	bool listening = false;

	LPFN_ACCEPTEX AcceptEx = nullptr;
	LPFN_GETACCEPTEXSOCKADDRS GetAcceptExSockaddrs = nullptr;

};

class winsock_state {
public:

	// worker threads to process the i/o completion port events
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
std::ostream& operator<<(std::ostream& out, no::ip_protocol protocol);

#endif
