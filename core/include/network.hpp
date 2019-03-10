#pragma once

#include "event.hpp"
#include "io.hpp"

namespace no {

class socket_container;

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

struct socket_events {
	message_event<io_stream> stream;
	message_event<io_stream> packet;
	message_event<socket_close_status> disconnect;
	message_event<int> accept;
};

template<typename P>
io_stream packet_stream(const P& packet) {
	io_stream stream;
	packetizer::start(stream);
	packet.write(stream);
	packetizer::end(stream);
	return stream;
}

int open_socket();
int open_socket(const std::string& address, int port);
void close_socket(int id);
void synchronize_socket(int id);
void synchronise_sockets();
bool bind_socket(int id, const std::string& address, int port);
bool listen_socket(int id);
bool increment_socket_accepts(int id);
void socket_send(int id, io_stream&& stream);
void broadcast(io_stream&& stream);
void broadcast(io_stream&& stream, int except_id);
socket_events& socket_event(int id);

template<typename P>
void send_packet(int id, const P& packet) {
	socket_send(id, std::move(packet_stream(packet)));
}

template<typename P>
void broadcast(const P& packet) {
	broadcast(packet_stream(packet));
}

template<typename P>
void broadcast(const P& packet, int except_id) {
	broadcast(packet_stream(packet), except_id);
}

}

std::ostream& operator<<(std::ostream& out, no::socket_close_status status);
