#pragma once

#include "character.hpp"
#include "io.hpp"
#include "math.hpp"

// undefined at end of file.
#define BEGIN_PACKET(NAME, TYPE) \
struct NAME { \
    NAME() = default; \
    NAME(no::io_stream& stream) { read(stream); } \
    static const int16_t type = TYPE;

#define END_PACKET() \
};

namespace packet {

namespace game {

BEGIN_PACKET(player_joined, 1)

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

END_PACKET()

BEGIN_PACKET(player_disconnected, 2)

int32_t player_instance_id = -1;

void write(no::io_stream& stream) const {
	stream.write(type);
	stream.write(player_instance_id);
}

void read(no::io_stream& stream) {
	player_instance_id = stream.read<int32_t>();
}

END_PACKET()

BEGIN_PACKET(move_to_tile, 3)

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

END_PACKET()

}

namespace lobby {

BEGIN_PACKET(connect_to_world, 1000)

int32_t world = 0;

void write(no::io_stream& stream) const {
	stream.write(type);
	stream.write(world);
}

void read(no::io_stream& stream) {
	world = stream.read<int32_t>();
}

END_PACKET()

}

namespace updates {

BEGIN_PACKET(version_check, 2000)

int32_t version = 0;
bool needs_assets = false;

void write(no::io_stream& stream) const {
	stream.write(type);
	stream.write(version);
	stream.write<int8_t>(needs_assets ? 1 : 0);
}

void read(no::io_stream& stream) {
	version = stream.read<int32_t>();
	needs_assets = (stream.read<int8_t>() != 0);
}

END_PACKET()

BEGIN_PACKET(file_transfer, 2001)

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

END_PACKET()

}

}

#undef BEGIN_PACKET
#undef END_PACKET
