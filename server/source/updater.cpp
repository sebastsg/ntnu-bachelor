#include "updater.hpp"
#include "assets.hpp"
#include "packets.hpp"

#include <filesystem>

client_updater::client_updater(no::io_socket& client) : client(client) {
	paths = no::entries_in_directory(no::asset_path(""), no::entry_inclusion::only_files, true);
	paths.push_back("Einheri.exe");
	total_files = (int)paths.size();
}

client_updater::client_updater(client_updater&& that) : client(that.client) {
	std::swap(paths, that.paths);
	std::swap(done, that.done);
}

client_updater& client_updater::operator=(client_updater&& that) {
	std::swap(client, that.client);
	std::swap(paths, that.paths);
	std::swap(done, that.done);
	return *this;
}

void client_updater::update() {
	if (paths.empty()) {
		done = true;
		return;
	}
	no::io_stream stream;
	no::file::read(paths.back(), stream);
	size_t packet_size = stream.size_left_to_read();
	if (packet_size == 0) {
		WARNING("Empty file: " << paths.back());
		return;
	}
	packet::updates::file_transfer packet;
	packet.name = paths.back();
	if (packet.name.find(no::asset_path("")) == 0) {
		packet.name = packet.name.substr(no::asset_path("").size());
	}
	current_file++;
	packet.file = current_file;
	packet.total_files = total_files;
	packet.offset = (int64_t)stream.read_index();
	packet.total_size = (int64_t)stream.write_index();
	packet.data.resize(packet_size);
	stream.read(packet.data.data(), packet_size);
	client.send_async(no::packet_stream(packet));
	paths.pop_back();
}

bool client_updater::is_done() const {
	return done;
}
