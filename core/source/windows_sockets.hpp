#pragma once

#include <winsock2.h>
#include <ws2tcpip.h>
#include <MSWSock.h>

#include "platform.hpp"
#include "network.hpp"
#include "debug.hpp"
#include "io.hpp"

#include <mutex>
#include <unordered_set>

#if PLATFORM_WINDOWS

namespace no {

enum class iocp_operation { invalid, send, receive, accept, connect, close };

class iocp_data {
public:

	WSAOVERLAPPED overlapped = {};
	DWORD bytes = 0;

	iocp_data() = default;
	iocp_data(const iocp_data&) = delete;
	iocp_data(iocp_data&&);

	virtual ~iocp_data() = default;

	iocp_data& operator=(const iocp_data&) = delete;
	iocp_data& operator=(iocp_data&&);

	virtual iocp_operation operation() const;

};

class iocp_send_data : public iocp_data {
public:

	WSABUF buffer = { 0, nullptr };
	socket_location location = { nullptr };

	iocp_send_data() = default;
	iocp_send_data(const iocp_send_data&) = delete;
	iocp_send_data(iocp_send_data&&);

	iocp_send_data& operator=(const iocp_send_data&) = delete;
	iocp_send_data& operator=(iocp_send_data&&);

	iocp_operation operation() const override;

};

class iocp_receive_data : public iocp_data {
public:

	static const size_t buffer_size = 262144; // 256 KiB

	char data[buffer_size];
	WSABUF buffer = { buffer_size, data };
	socket_location location = { nullptr };

	iocp_receive_data() = default;
	iocp_receive_data(const iocp_receive_data&) = delete;
	iocp_receive_data(iocp_receive_data&&);

	iocp_receive_data& operator=(const iocp_receive_data&) = delete;
	iocp_receive_data& operator=(iocp_receive_data&&);

	iocp_operation operation() const override;

};

class iocp_accept_data : public iocp_data {
public:

	// the buffer size for the local and remote address must be 16 bytes more than the
	// size of the sockaddr structure because the addresses are written in an internal format.
	// https://msdn.microsoft.com/en-us/library/windows/desktop/ms737524(v=vs.85).aspx
	static const size_t padded_addr_size = (sizeof(SOCKADDR_IN) + 16);
	static const size_t buffer_size = padded_addr_size * 2;

	char data[buffer_size];
	WSABUF buffer = { buffer_size, data };
	listener_socket* listener = nullptr;
	int id = -1;

	iocp_accept_data() = default;
	iocp_accept_data(const iocp_accept_data&) = delete;
	iocp_accept_data(iocp_accept_data&&);

	iocp_accept_data& operator=(const iocp_accept_data&) = delete;
	iocp_accept_data& operator=(iocp_accept_data&&);

	iocp_operation operation() const override;

};

class iocp_close_data : public iocp_data {
public:

	iocp_operation operation() const override;

};

class winsock_socket {
public:

	packetizer receive_packetizer;
	std::mutex mutex;

	struct {
		std::unordered_set<iocp_send_data*> send;
		std::unordered_set<iocp_receive_data*> receive;
		std::unordered_set<iocp_accept_data*> accept;
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
	void get_accept_sockaddrs(iocp_accept_data& data);
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

	winsock_state();
	winsock_state(const winsock_state&) = delete;
	winsock_state(winsock_state&&) = delete;

	~winsock_state();

	winsock_state& operator=(const winsock_state&) = delete;
	winsock_state& operator=(winsock_state&&) = delete;

	int create_socket();
	bool destroy_socket(int id);

	void create_completion_port();
	void destroy_completion_port();

	void print_error(int error, const std::string& funcsig, int line, int log) const;

	int status() const;
	HANDLE port_handle() const;
	winsock_socket& socket(int id);

private:

	WSADATA wsa_data;
	HANDLE io_port = INVALID_HANDLE_VALUE;
	int setup_result = -1;
	int socket_id_counter = 0;
	std::vector<std::thread> threads;

	// todo: this shouldn't use a dynamic container 
	//       a socket can technically change address here while an event has already been posted
	//       that will result in a crash or undefined behaviour
	std::unordered_map<int, winsock_socket> sockets;
};

}

std::ostream& operator<<(std::ostream& out, no::iocp_operation operation);
std::ostream& operator<<(std::ostream& out, no::ip_protocol protocol);

#endif
