#pragma once

#include <winsock2.h>
#include <ws2tcpip.h>
#include <MSWSock.h>

#include "platform.hpp"
#include "network.hpp"

#include <mutex>
#include <unordered_set>

#if PLATFORM_WINDOWS

namespace no {

enum class iocp_operation { invalid, send, receive, accept, close };

template<iocp_operation O>
struct iocp_data {
	WSAOVERLAPPED overlapped = {};
	DWORD bytes = 0;
	iocp_operation operation = O;
};

struct iocp_send_data : iocp_data<iocp_operation::send> {
	WSABUF buffer = { 0, nullptr };
};

struct iocp_receive_data : iocp_data<iocp_operation::receive> {
	static const size_t buffer_size = 262144; // 256 KiB

	char data[buffer_size];
	WSABUF buffer = { buffer_size, data };
};

struct iocp_accept_data : iocp_data<iocp_operation::accept> {
	// the buffer size for the local and remote address must be 16 bytes more than the
	// size of the sockaddr structure because the addresses are written in an internal format.
	// https://msdn.microsoft.com/en-us/library/windows/desktop/ms737524(v=vs.85).aspx
	static const size_t padded_addr_size = (sizeof(SOCKADDR_IN) + 16);
	static const size_t buffer_size = padded_addr_size * 2;

	char data[buffer_size];
	WSABUF buffer = { buffer_size, data };
	int accepted_id = -1;
};

struct iocp_close_data : iocp_data<iocp_operation::close> {};

struct winsock_socket {

	bool alive = false;
	SOCKET handle = INVALID_SOCKET;
	bool connected = false;
	bool listening = false;
	packetizer receive_packetizer;
	std::vector<io_stream> queued_packets;
	WSABUF received = { 0, nullptr }; // stores received buffer until a packet is recognized
	addrinfo hints = {};
	SOCKADDR_IN addr = {};
	int addr_size = sizeof(addr);

	struct {
		std::unordered_set<iocp_send_data*> send;
		std::unordered_set<iocp_receive_data*> receive;
		std::unordered_set<iocp_accept_data*> accept;
	} io;

	struct {
		event_message_queue<io_stream> stream;
		event_message_queue<io_stream> packet;
		event_message_queue<socket_close_status> disconnect;
		event_message_queue<int> accept;
	} sync;

	socket_events events;

	winsock_socket();

};

struct winsock_state {

	static constexpr int max_broadcasts_per_sync = 4192;

	LPFN_ACCEPTEX AcceptEx = nullptr;
	LPFN_GETACCEPTEXSOCKADDRS GetAcceptExSockaddrs = nullptr;

	WSADATA wsa_data = {};
	HANDLE io_port = INVALID_HANDLE_VALUE;

	std::vector<winsock_socket> sockets;
	std::vector<std::unique_ptr<std::mutex>> mutexes;
	std::vector<std::thread> threads;

	// sockets to destroy in synchronise
	std::vector<int> destroy_queue;

	// to keep broadcast packets alive until sync (instead of 1 copy per socket)
	int broadcast_count = 0;
	io_stream queued_packets[max_broadcasts_per_sync]; // 20 * 4192 = 82 KiB

};

}

std::ostream& operator<<(std::ostream& out, no::iocp_operation operation);

#endif
