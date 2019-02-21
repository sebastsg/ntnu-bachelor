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

io_socket* socket_location::get() {
	if (container) {
		return &container->sockets[index];
	}
	return socket;
}

size_t socket_container::create() {
	size_t index = 0;
	if (available_sockets.size() == 0) {
		sockets.push_back({});
		index = sockets.size() - 1;
	} else {
		index = available_sockets.front();
		available_sockets.pop();
	}
	// reset the socket, as it's not done during destruction
	sockets[index] = {};
	active_sockets.push_back(index);
	return index;
}

void socket_container::destroy(size_t index) {
	// note: never do this: sockets[index] = {};
	//       it will destroy the event queue before it has finished
	//       the socket will be reset when it is used again
	for (size_t i = 0; i < active_sockets.size(); i++) {
		if (active_sockets[i] == index) {
			active_sockets.erase(active_sockets.begin() + i);
			break;
		}
	}
	available_sockets.push(index);
}

void socket_container::synchronise() {
	for (size_t i : active_sockets) {
		sockets[i].synchronise();
	}
}

void connection_establisher::listen(const std::string& host, int port) {
	listener.bind(host, port);
	listener.listen();
	listener.events.accept.listen([this](const listener_socket::accept_message& accept) {
		INFO("Accepted client.");
		sync.accept.emplace_and_push(accept);
	});
}

void connection_establisher::synchronise() {
	listener.synchronise();
	sync.accept.all([this](const listener_socket::accept_message& message) {
		// there is no need to lock the socket mutex here, as we know it has no currently pending i/o events

		// create a new socket or reuse an existing one
		size_t index = container->create();
		io_socket* socket = &container->sockets[index];
		socket->id = message.id;
		socket->location.container = container;
		socket->location.index = index;

		// ensure that we are always the first in the queue when the socket disconnects, which means
		// that when the server receives the disconnect event, they cannot accidentally broadcast() to it
		socket->events.disconnect.listen([this, index](const io_socket::disconnect_message& disconnect) {
			container->destroy(index);
		});

		// note: the event listeners expect the socket to NOT be locked, as it is not an io_socket event
		events.established.emit({ index });

		// receive() call must always be last, as threads may lock the socket's mutex during the call
		socket->receive();
	});
}

}

std::ostream& operator<<(std::ostream& out, no::socket_close_status status) {
	switch (status) {
	case no::socket_close_status::disconnected_gracefully: return out << "Disconnected gracefully";
	case no::socket_close_status::connection_reset: return out << "Connection reset";
	case no::socket_close_status::not_connected: return out << "Not connected";
	case no::socket_close_status::unknown: return out << "Unknown";
	default: return out << "Invalid";
	}
}

std::ostream& operator<<(std::ostream& out, no::address_family family) {
	switch (family) {
	case no::address_family::inet4: return out << "IPv4";
	case no::address_family::inet6: return out << "IPv6";
	default: return out << "Invalid";
	}
}
