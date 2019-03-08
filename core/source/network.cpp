#include "network.hpp"
#include "windows_sockets.hpp"

namespace no {

void packetizer::start(io_stream& stream) {
	stream.write(magic);
	stream.write<body_size_type>(0); // offset for the size
}

void packetizer::end(io_stream& stream) {
	if (header_size > stream.write_index()) {
		return;
	}
	// go back to the beginning and write the size
	size_t size = stream.write_index() - header_size;
	stream.set_write_index(sizeof(magic_type));
	stream.write<body_size_type>(size);
	stream.move_write_index(size);
}

packetizer::packetizer() {
	stream.allocate(1024 * 1024 * 10); // prevent resizes per sync. todo: improve, because this is lazy
}

char* packetizer::data() {
	return stream.data();
}

size_t packetizer::write_index() const {
	return stream.write_index();
}

void packetizer::write(char* data, size_t size) {
	stream.write(data, size);
}

io_stream packetizer::next() {
	if (header_size > stream.size_left_to_read()) {
		return {};
	}
	auto body_size = stream.peek<body_size_type>(sizeof(magic_type));
	if (header_size + body_size > stream.size_left_to_read()) {
		return {};
	}
	if (stream.peek<magic_type>() != magic) {
		WARNING("Skipping magic... " << stream.peek<magic_type>() << " != " << magic);
		stream.move_read_index(1); // no point in reading the same magic again
		return {};
	}
	stream.move_read_index(header_size);
	auto body_begin = stream.at_read();
	stream.move_read_index(body_size);
	return { body_begin, body_size, io_stream::construct_by::shallow_copy };
}

void packetizer::clean() {
	stream.shift_read_to_begin();
}

socket_location::socket_location(io_socket* socket) : socket(socket) {

}

socket_location::socket_location(size_t index, socket_container& container) : index(index), container(&container) {

}

io_socket* socket_location::find() const {
	if (container) {
		return &container->at(index);
	}
	return socket;
}

void socket_container::broadcast(io_stream&& stream) {
	auto& packet = queued_packets.emplace_back(std::move(stream));
	for (size_t i : active_sockets) {
		sockets[i].send({ packet.data(), packet.size(), io_stream::construct_by::shallow_copy });
	}
}

void socket_container::broadcast(io_stream&& stream, size_t except_index) {
	auto& packet = queued_packets.emplace_back(std::move(stream));
	for (size_t i : active_sockets) {
		if (i != except_index) {
			sockets[i].send({ packet.data(), packet.size(), io_stream::construct_by::shallow_copy });
		}
	}
}

io_socket& socket_container::at(size_t index) {
	return sockets[index];
}

const io_socket& socket_container::at(size_t index) const {
	return sockets[index];
}

io_socket& socket_container::operator[](size_t index) {
	return sockets[index];
}

const io_socket& socket_container::operator[](size_t index) const {
	return sockets[index];
}

size_t socket_container::create(int id) {
	size_t index = 0;
	if (available_sockets.size() == 0) {
		index = sockets.size();
		sockets.emplace_back(id, socket_location{ index, *this });
	} else {
		index = available_sockets.front();
		available_sockets.pop();
		sockets[index] = { id, { index, *this } };
	}
	active_sockets.push_back(index);
	return index;
}

void socket_container::destroy(size_t index) {
	destroy_queue.push_back(index);
}

void socket_container::synchronise() {
	for (size_t i : active_sockets) {
		sockets[i].synchronise();
	}
	queued_packets.clear();
	destroy_queued();
}

void socket_container::destroy_queued() {
	for (size_t destroy_index : destroy_queue) {
		for (size_t i = 0; i < active_sockets.size(); i++) {
			if (active_sockets[i] == destroy_index) {
				active_sockets.erase(active_sockets.begin() + i);
				break;
			}
		}
		available_sockets.push(destroy_index);
		sockets[destroy_index] = {};
	}
	destroy_queue.clear();
}

connection_establisher::connection_establisher(socket_container& container) : container(container) {

}

void connection_establisher::listen(const std::string& host, int port) {
	listener.bind(host, port);
	listener.listen();
	listener.events.accept.listen([this](const listener_socket::accept_event& event) {
		sync.accept.emplace_and_push(event);
	});
}

void connection_establisher::synchronise() {
	listener.synchronise();
	sync.accept.all([this](const listener_socket::accept_event& event) {
		// no need to lock the socket mutex. it has no currently pending i/o events
		size_t index = container.create(event.id);
		io_socket& socket = container[index];

		// ensure that we are always the first in the queue when the socket disconnects, which means
		// that when the server receives the disconnect event, they cannot accidentally broadcast() to it
		socket.events.disconnect.listen([this, index](const socket_close_status& status) {
			container.destroy(index);
		});

		events.establish.emit(index);

		// receive() call must always be last, as threads may lock the socket's mutex during the call
		socket.receive();
	});
}

}

std::ostream& operator<<(std::ostream& out, no::socket_close_status status) {
	switch (status) {
	case no::socket_close_status::disconnected_gracefully: return out << "Disconnected gracefully";
	case no::socket_close_status::connection_reset: return out << "Connection reset";
	case no::socket_close_status::not_connected: return out << "Not connected";
	case no::socket_close_status::unknown: return out << "Unknown";
	default: return out << "Invalid (" << (int)status << ")";
	}
}

std::ostream& operator<<(std::ostream& out, no::address_family family) {
	switch (family) {
	case no::address_family::inet4: return out << "IPv4";
	case no::address_family::inet6: return out << "IPv6";
	default: return out << "Invalid (" << (int)family << ")";
	}
}
