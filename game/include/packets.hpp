#pragma once

#include "character.hpp"
#include "io.hpp"
#include "math.hpp"

struct move_to_tile_packet {

	static const int16_t type = 1;

	int64_t timestamp = 0;
	no::vector2i tile = -1;
	int32_t player_instance_id = -1;

	void write(no::io_stream& stream) const {
		stream.write(type);
		stream.write(timestamp);
		stream.write(tile);
		stream.write(player_instance_id);
	}

	void read(no::io_stream& stream) {
		timestamp = stream.read<int64_t>();
		tile = stream.read<no::vector2i>();
		player_instance_id = stream.read<int32_t>();
	}

};

struct player_joined_packet {

	static const int16_t type = 2;

	uint8_t is_me = 0;
	character_object player;

	void write(no::io_stream& stream) const {
		stream.write(type);
		stream.write(is_me);
		player.write(stream);
	}

	void read(no::io_stream& stream) {
		is_me = stream.read<uint8_t>();
		player.read(stream);
	}

};

struct player_disconnected_packet {

	static const int16_t type = 3;

	int32_t player_instance_id = -1;

	void write(no::io_stream& stream) const {
		stream.write(type);
		stream.write(player_instance_id);
	}

	void read(no::io_stream& stream) {
		player_instance_id = stream.read<int32_t>();
	}

};

struct version_packet {

	static const int16_t type = 1000;

	int32_t version = 0;

	void write(no::io_stream& stream) const {
		stream.write(type);
		stream.write(version);
	}

	void read(no::io_stream& stream) {
		version = stream.read<int32_t>();
	}

};

struct file_transfer_packet {
	
	static const int16_t type = 1001;

	std::string name;
	int32_t file = 0;
	int32_t total_files = 0;
	int64_t offset = 0;
	int64_t total_size = 0;
	std::vector<char> data;

	void write(no::io_stream& stream) const {
		stream.write(type);
		stream.write(name);
		stream.write(file);
		stream.write(total_files);
		stream.write(offset);
		stream.write(total_size);
		stream.write_array<char>(data);
	}

	void read(no::io_stream& stream) {
		name = stream.read<std::string>();
		file = stream.read<int32_t>();
		total_files = stream.read<int32_t>();
		offset = stream.read<int64_t>();
		total_size = stream.read<int64_t>();
		data = stream.read_array<char>();
	}
	
};
