#include "network.hpp"
#include "windows_sockets.hpp"

namespace no {

io_socket* socket_location::get() {
	if (container) {
		return &container->sockets[index];
	}
	return socket;
}

void packetizer::start(io_stream& stream) {
	stream.write<uint16_t>(magic);
	stream.write<uint32_t>(0); // offset for the size
}

void packetizer::end(io_stream& stream) {
	size_t header_size = sizeof(magic) + sizeof(uint32_t);
	if (header_size > stream.write_index()) {
		return;
	}
	// go back to the beginning and write the size
	size_t size = stream.write_index() - header_size;
	stream.set_write_index(sizeof(magic));
	stream.write<uint32_t>(size);
	stream.move_write_index(size);
}

void packetizer::write(char* data, size_t size) {
	stream.write(data, size);
}

void packetizer::clean() {
	stream.shift_read_to_begin();
	packet = {};
	packet_size = 0;
	found_magic = false;
}

bool packetizer::parse_next() {
	if (sizeof(magic) > stream.write_index()) {
		return false;
	}
	if (!found_magic) {
		uint16_t read_magic = stream.read<uint16_t>();
		if (read_magic != magic) {
			return false;
		}
		found_magic = true;
	}
	// check if the size is available
	if (sizeof(uint32_t) > stream.write_index() + sizeof(magic)) {
		return false;
	}

	// read the size if we haven't already
	if (packet_size == 0) {
		packet_size = stream.read<uint32_t>();
		if (packet_size == 0) {
			return true; // well, this is odd... but it seems to be a valid packet
		}
	}

	// make sure we have received the entire packet
	if (packet_size > stream.write_index() + sizeof(uint32_t) + sizeof(magic)) {
		return false;
	}

	// set the data as the current read position
	packet = { stream.at_read(), packet_size, io_stream::construct_by::shallow_copy };

	// since we technically have read the buffer now, we should add to our read position
	stream.move_read_index(packet_size - sizeof(magic));

	// per-packet reset
	packet_size = 0;
	found_magic = false;

	return true;
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
