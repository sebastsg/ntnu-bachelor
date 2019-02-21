#include "updater.hpp"
#include "window.hpp"
#include "game.hpp"

#include <filesystem>

file_transfer::file_transfer(const std::string& path) : path(path) {

}

void file_transfer::update(const file_transfer_packet& packet) {
	stream.resize_if_needed((size_t)packet.total_size);
	stream.set_write_index((size_t)packet.offset);
	stream.write(packet.data.data(), packet.data.size());
	total_received += packet.data.size();
	if (total_received >= packet.total_size) {
		stream.set_write_index(total_received);
		// todo: improve this, but doesn't matter right now
		if (path.find("Einheri") == 0) {
			std::filesystem::rename("Einheri.exe", "einheri.old");
			no::file::write(path, stream);
		} else {
			no::file::write(full_path(), stream);
		}
		completed = true;
	}
}

std::string file_transfer::relative_path() const {
	return path;
}

std::string file_transfer::full_path() const {
	return no::asset_path(path);
}

bool file_transfer::is_completed() const {
	return completed;
}

updater_state::updater_state() {
	window().set_swap_interval(no::swap_interval::immediate);
	set_synchronization(no::draw_synchronization::always);

	server.connect("game.einheri.xyz", 7524); // todo: config file

	version_packet packet;
	packet.version = client_version;
	server.send_async(no::packet_stream(packet));

	previous_packet.start();

	server.events.receive_stream.listen([this](const no::io_socket::receive_stream_message& event) {
		MESSAGE("Received " << event.packet.size_left_to_read() << " bytes");
	});

	server.events.receive_packet.listen([this](const no::io_socket::receive_packet_message& event) {
		no::io_stream stream = { event.packet.data(), event.packet.size(), no::io_stream::construct_by::shallow_copy };
		int16_t type = stream.read<int16_t>();
		switch (type) {
		case version_packet::type:
		{
			version_packet packet;
			packet.read(stream);
			INFO("Current version: " << client_version << ". Newest version: " << packet.version);
			if (client_version == packet.version) {
				if (std::filesystem::is_regular_file("einheri.old")) {
					MESSAGE("Removing old client");
					std::filesystem::remove("einheri.old");
				}
				if (std::filesystem::is_directory(no::asset_path(""))) {
					change_state<game_state>();
				}
			}
			break;
		}
		case file_transfer_packet::type:
		{
			previous_packet.start();
			file_transfer_packet packet;
			packet.read(stream);
			auto& transfer = transfer_for_path(packet.name);
			transfer.update(packet);
			if (transfer.is_completed()) {
				erase_transfer(transfer.relative_path());
				completed_transfers++;
				INFO("Completed transfer " << packet.file << " / " << packet.total_files << " (" << packet.total_size << " bytes)\n" << packet.name);
				if (completed_transfers >= packet.total_files) {
					no::platform::relaunch();
				}
			}
			break;
		}
		default:
			WARNING("Received unknown packet: " << type);
			break;
		}
	});
}

updater_state::~updater_state() {
	
}

void updater_state::update() {
	server.synchronise();
	if (previous_packet.seconds() > 10) {
		CRITICAL("Failed to update.");
		stop();
	}
}

void updater_state::draw() {

}

file_transfer& updater_state::transfer_for_path(const std::string& path) {
	for (auto& transfer : transfers) {
		if (transfer.relative_path() == path) {
			return transfer;
		}
	}
	return transfers.emplace_back(path);
}

void updater_state::erase_transfer(const std::string& path) {
	for (size_t i = 0; i < transfers.size(); i++) {
		if (transfers[i].relative_path() == path) {
			transfers.erase(transfers.begin() + i);
			break;
		}
	}
}
