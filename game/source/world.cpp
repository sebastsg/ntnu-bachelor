#include "world.hpp"

world_terrain::world_terrain(world_state& world) : world(world) {
	tile_array.resize_and_reset(128, {});
}

bool world_terrain::is_out_of_bounds(no::vector2i tile) const {
	return tile_array.is_out_of_bounds(tile.x, tile.y);
}

float world_terrain::elevation_at(no::vector2i tile) const {
	return tile_array.at(tile.x, tile.y).height;
}

void world_terrain::set_elevation_at(no::vector2i tile, float elevation) {
	tile_array.at(tile.x, tile.y).height = elevation;
	dirty = true;
}

void world_terrain::elevate_tile(no::vector2i tile, float amount) {
	if (is_out_of_bounds(tile)) {
		return;
	}
	tile_array.at(tile.x, tile.y).height += amount;
	tile_array.at(tile.x + 1, tile.y).height += amount;
	tile_array.at(tile.x, tile.y + 1).height += amount;
	tile_array.at(tile.x + 1, tile.y + 1).height += amount;
	dirty = true;
}

void world_terrain::set_tile_type(no::vector2i tile, int type) {
	if (is_out_of_bounds(tile)) {
		return;
	}
	tile_array.at(tile.x, tile.y).type = type;
	dirty = true;
}

no::vector2i world_terrain::offset() const {
	return tile_array.position();
}

no::vector2i world_terrain::size() const {
	return { tile_array.columns(), tile_array.rows() };
}

const no::shifting_2d_array<world_tile>& world_terrain::tiles() const {
	return tile_array;
}

void world_terrain::read(no::io_stream& stream) {
	tile_array.read(stream, 1024, {});
	dirty = true;
}

void world_terrain::write(no::io_stream& stream) const {
	tile_array.write(stream, 1024, {});
}

void world_terrain::shift_left(no::io_stream& stream) {
	tile_array.shift_left(stream, 1024, {});
	dirty = true;
}

void world_terrain::shift_right(no::io_stream& stream) {
	tile_array.shift_right(stream, 1024, {});
	dirty = true;
}

void world_terrain::shift_up(no::io_stream& stream) {
	tile_array.shift_up(stream, 1024, {});
	dirty = true;
}

void world_terrain::shift_down(no::io_stream& stream) {
	tile_array.shift_down(stream, 1024, {});
	dirty = true;
}

bool world_terrain::is_dirty() const {
	return dirty;
}

void world_terrain::set_clean() {
	dirty = false;
}

no::vector2i world_terrain::global_to_local_tile(no::vector2i tile) const {
	return tile - tile_array.position();
}

no::vector2i world_terrain::local_to_global_tile(no::vector2i tile) const {
	return tile + tile_array.position();
}

world_state::world_state() : terrain(*this) {
	
}

world_state::~world_state() {
	for (auto& player : players) {
		delete player;
	}
	for (auto& decoration : decorations) {
		delete decoration;
	}
}

void world_state::update() {
	for (auto& player : players) {
		player->update();
	}
}

player_object* world_state::add_player(int id) {
	auto object = player(id);
	if (object) {
		return object;
	}
	object = new player_object(*this);
	players.push_back(object);
	object->player_id = id;
	return object;
}

void world_state::remove_player(int id) {
	for (size_t i = 0; i < players.size(); i++) {
		if (players[i]->player_id == id) {
			delete players[i];
			players.erase(players.begin() + i);
			break;
		}
	}
}

player_object* world_state::player(int id) {
	for (auto& player : players) {
		if (player->player_id == id) {
			return player;
		}
	}
	return nullptr;
}

decoration_object* world_state::add_decoration() {
	return decorations.emplace_back(new decoration_object(*this));
}

void world_state::remove_decoration(decoration_object* decoration) {
	for (size_t i = 0; i < decorations.size(); i++) {
		if (decorations[i] == decoration) {
			delete decoration;
			decorations.erase(decorations.begin() + i);
		}
	}
}

no::vector2i world_state::world_position_to_tile_index(float x, float z) const {
	return { (int)std::floor(x), (int)std::floor(z) };
}

no::vector3f world_state::tile_index_to_world_position(int x, int z) const {
	return { (float)x, 0.0f, (float)z };
}

int world_state::next_object_id() {
	return ++object_id_counter;
}
