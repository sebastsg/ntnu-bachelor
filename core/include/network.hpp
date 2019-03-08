#pragma once

#include "event.hpp"
#include "io.hpp"
#include "debug.hpp"

#include <queue>

namespace no {

class socket_container;
class io_socket;

enum class ip_protocol { tcp, udp };
enum class address_family { inet4, inet6 };
enum class socket_close_status { disconnected_gracefully, connection_reset, not_connected, unknown };

void start_network();
void stop_network();

class packetizer {
public:

	static void start(io_stream& stream);
	static void end(io_stream& stream);

	packetizer();

	char* data();
	size_t write_index() const;
	void write(char* data, size_t size);
	io_stream next();
	void clean();

private:

	using magic_type = uint32_t;
	using body_size_type = uint32_t;

	static const magic_type magic = 'NFWK';
	static const size_t header_size = sizeof(magic_type) + sizeof(body_size_type);

	io_stream stream;

};

template<typename P>
io_stream packet_stream(const P& packet) {
	io_stream stream;
	packetizer::start(stream);
	packet.write(stream);
	packetizer::end(stream);
	return stream;
}

class abstract_socket {
public:

	abstract_socket() = default;
	abstract_socket(int id);
	abstract_socket(const abstract_socket&) = delete;
	abstract_socket(abstract_socket&&);

	virtual ~abstract_socket() = default;

	abstract_socket& operator=(const abstract_socket&) = delete;
	abstract_socket& operator=(abstract_socket&&);

	bool disconnect();
	bool set_protocol(ip_protocol protocol);
	bool set_address_family(address_family family);

	int id() const;

protected:
	
	int socket_id = -1;

};

class socket_location {
public:

	socket_location(io_socket* socket);
	socket_location(size_t index, socket_container& container);

	io_socket* find() const;

private:

	// index to access the socket through container
	size_t index = 0;
	socket_container* container = nullptr;

	// if we have no container, store the socket's this pointer
	io_socket* socket = nullptr;

};

class io_socket : public abstract_socket {
public:

	struct {
		message_event<io_stream> receive_stream;
		message_event<io_stream> receive_packet;
		message_event<socket_close_status> disconnect;
	} events;

	struct {
		event_message_queue<io_stream> receive_stream;
		event_message_queue<io_stream> receive_packet;
		event_message_queue<socket_close_status> disconnect;
	} sync;

	io_socket();
	io_socket(int id, const socket_location& location);
	io_socket(const io_socket&) = delete;
	io_socket(io_socket&&);

	io_socket& operator=(const io_socket&) = delete;
	io_socket& operator=(io_socket&&);

	void send(io_stream&& packet);
	
	template<typename P>
	void send(const P& packet) {
		send(packet_stream(packet));
	}
	
	bool receive();

	bool connect();
	bool connect(const std::string& address, int port);

	void synchronise();

private:

	bool send_synced(const io_stream& packet);

	socket_location location;
	std::vector<io_stream> queued_packets;

};

class listener_socket : public abstract_socket {
public:

	struct accept_event {
		int error = 0;
		int id = -1;
	};

	struct {
		message_event<accept_event> accept;
	} events;

	struct {
		event_message_queue<accept_event> accept;
	} sync;

	listener_socket() = default;
	listener_socket(const listener_socket&) = delete;
	listener_socket(listener_socket&&);

	listener_socket& operator=(const listener_socket&) = delete;
	listener_socket& operator=(listener_socket&&);

	bool bind(const std::string& address, int port);

	// bind the socket to a port
	bool bind();

	bool listen();

	// increment the number of asynchronous accept calls
	// if the I/O limit has been reached, this function will return false
	// if the accept call itself failed, we also return false
	bool accept();

	void synchronise();

};

class socket_container {
public:

	void broadcast(io_stream&& stream);
	void broadcast(io_stream&& stream, size_t except_index);

	template<typename P>
	void broadcast(const P& packet) {
		broadcast(packet_stream(packet));
	}

	template<typename P>
	void broadcast(const P& packet, size_t except_index) {
		broadcast(packet_stream(packet), except_index);
	}

	io_socket& at(size_t index);
	const io_socket& at(size_t index) const;

	io_socket& operator[](size_t index);
	const io_socket& operator[](size_t index) const;

	// create a new active (not open) socket. returns index
	size_t create(int id);

	// destroy an active socket
	void destroy(size_t index);

	void synchronise();
	
private:

	void destroy_queued();

	// all the sockets contained. not guaranteed to be active
	std::vector<io_socket> sockets;

	// which indices in the sockets vector are currently unused, and can be reused
	std::queue<size_t> available_sockets;

	// which indices in the sockets vector are currently in use
	std::vector<size_t> active_sockets;

	// to keep memory alive
	std::vector<io_stream> queued_packets;

	// indices of sockets to destroy in synchronise
	std::vector<size_t> destroy_queue;

};

class connection_establisher {
public:

	struct establish_event {
		size_t index = 0;
	};

	struct {
		message_event<establish_event> establish;
	} events;

	connection_establisher(socket_container& container);

	void listen(const std::string& host, int port);

	void synchronise();

private:

	listener_socket listener;
	socket_container& container;

	struct {
		event_message_queue<listener_socket::accept_event> accept;
	} sync;

};

}

std::ostream& operator<<(std::ostream& out, no::socket_close_status status);
std::ostream& operator<<(std::ostream& out, no::address_family family);
