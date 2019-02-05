#pragma once

#include "io.hpp"
#include "math.hpp"

struct move_to_tile_packet {

	static const int16_t type = 1;

	int64_t timestamp = 0;
	no::vector2i tile = -1;
	int32_t player_id = -1;

	void write(no::io_stream& stream) {
		stream.write(type);
		stream.write(timestamp);
		stream.write(tile);
		stream.write(player_id);
	}

	void read(no::io_stream& stream) {
		timestamp = stream.read<int64_t>();
		tile = stream.read<no::vector2i>();
		player_id = stream.read<int32_t>();
	}

};

struct player_joined_packet {

	static const int16_t type = 2;

	uint8_t is_me = 0;
	int32_t player_id = -1;
	no::vector2i tile = -1;

	void write(no::io_stream& stream) {
		stream.write(type);
		stream.write(is_me);
		stream.write(player_id);
		stream.write(tile);
	}

	void read(no::io_stream& stream) {
		is_me = stream.read<uint8_t>();
		player_id = stream.read<int32_t>();
		tile = stream.read<no::vector2i>();
	}

};

struct player_disconnected_packet {

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
