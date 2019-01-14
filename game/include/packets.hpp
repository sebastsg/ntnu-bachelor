#pragma once

#include "io.hpp"

class move_to_tile_packet {
public:

	static const int16_t type = 1;

	int64_t timestamp = 0;
	int32_t tile_x = -1;
	int32_t tile_z = -1;
	int32_t player_id = -1;

	void write(no::io_stream& stream) {
		stream.write(type);
		stream.write(timestamp);
		stream.write(tile_x);
		stream.write(tile_z);
		stream.write(player_id);
	}

	void read(no::io_stream& stream) {
		timestamp = stream.read<int64_t>();
		tile_x = stream.read<int32_t>();
		tile_z = stream.read<int32_t>();
		player_id = stream.read<int32_t>();
	}

};

class session_packet {
public:

	static const int16_t type = 2;

	uint8_t is_me = 0;
	int32_t player_id = -1;
	int32_t tile_x = -1;
	int32_t tile_z = -1;

	void write(no::io_stream& stream) {
		stream.write(type);
		stream.write(is_me);
		stream.write(player_id);
		stream.write(tile_x);
		stream.write(tile_z);
	}

	void read(no::io_stream& stream) {
		is_me = stream.read<uint8_t>();
		player_id = stream.read<int32_t>();
		tile_x = stream.read<int32_t>();
		tile_z = stream.read<int32_t>();
	}

};

class player_disconnected_packet {
public:

	static const int16_t type = 3;

	int32_t player_id = -1;

	void write(no::io_stream& stream) {
		stream.write(type);
		stream.write(player_id);
	}

	void read(no::io_stream& stream) {
		player_id = stream.read<int32_t>();
	}

};
