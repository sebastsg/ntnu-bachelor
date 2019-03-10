#include "network.hpp"

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
