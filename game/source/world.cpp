#include "world.hpp"
#include "assets.hpp"

world_autotiler::world_autotiler() {
	add_main(grass);
	add_main(dirt);
	add_main(water);
	row++;
	row += 2;
	add_group(grass, dirt);
	add_group(grass, water);
}

void world_autotiler::add_main(int tile) {
	uv_indices[packed_corners(tile, tile, tile, tile)] = { tile, 0 };
}

void world_autotiler::add_group(int primary, int secondary) {
	uv_indices[packed_corners(primary, primary, primary, secondary)] = { 0, row };
	uv_indices[packed_corners(primary, primary, secondary, primary)] = { 1, row };
	uv_indices[packed_corners(primary, secondary, secondary, primary)] = { 2, row };
	uv_indices[packed_corners(secondary, primary, primary, secondary)] = { 3, row };
	uv_indices[packed_corners(secondary, secondary, secondary, primary)] = { 4, row };
	uv_indices[packed_corners(secondary, secondary, primary, primary)] = { 5, row };
	uv_indices[packed_corners(secondary, secondary, primary, secondary)] = { 6, row };
	uv_indices[packed_corners(secondary, primary, secondary, primary)] = { 7, row };
	row++;
	uv_indices[packed_corners(primary, secondary, primary, primary)] = { 0, row };
	uv_indices[packed_corners(secondary, primary, primary, primary)] = { 1, row };
	uv_indices[packed_corners(secondary, primary, primary, secondary)] = { 2, row };
	uv_indices[packed_corners(primary, secondary, secondary, primary)] = { 3, row };
	uv_indices[packed_corners(secondary, primary, secondary, secondary)] = { 4, row };
	uv_indices[packed_corners(primary, primary, secondary, secondary)] = { 5, row };
	uv_indices[packed_corners(primary, secondary, secondary, secondary)] = { 6, row };
	uv_indices[packed_corners(primary, secondary, primary, secondary)] = { 7, row };
	row++;
}

uint32_t world_autotiler::packed_corners(int top_left, int top_right, int bottom_left, int bottom_right) const {
	uint32_t result = 0;
	result += (uint32_t)(top_left & 0xFF) << 24;
	result += (uint32_t)(top_right & 0xFF) << 16;
	result += (uint32_t)(bottom_left & 0xFF) << 8;
	result += (uint32_t)(bottom_right & 0xFF);
	return result;
}

no::vector2i world_autotiler::uv_index(uint32_t corners) const {
	auto it = uv_indices.find(corners);
	return it != uv_indices.end() ? it->second : 0;
}

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
	// top left
	tile -= 1;
	if (!is_out_of_bounds(tile)) {
		tile_array.at(tile.x, tile.y).corners[3] = type;
	}
	// top middle
	tile.x++;
	if (!is_out_of_bounds(tile)) {
		tile_array.at(tile.x, tile.y).corners[2] = type;
		tile_array.at(tile.x, tile.y).corners[3] = type;
	}
	// top right
	tile.x++;
	if (!is_out_of_bounds(tile)) {
		tile_array.at(tile.x, tile.y).corners[2] = type;
	}
	// middle left
	tile.x -= 2;
	tile.y++;
	if (!is_out_of_bounds(tile)) {
		tile_array.at(tile.x, tile.y).corners[1] = type;
		tile_array.at(tile.x, tile.y).corners[3] = type;
	}
	// middle
	tile.x++;
	if (!is_out_of_bounds(tile)) {
		tile_array.at(tile.x, tile.y).set(type);
	}
	// middle right
	tile.x++;
	if (!is_out_of_bounds(tile)) {
		tile_array.at(tile.x, tile.y).corners[0] = type;
		tile_array.at(tile.x, tile.y).corners[2] = type;
	}
	// bottom left
	tile.x -= 2;
	tile.y++;
	if (!is_out_of_bounds(tile)) {
		tile_array.at(tile.x, tile.y).corners[1] = type;
	}
	// bottom middle
	tile.x++;
	if (!is_out_of_bounds(tile)) {
		tile_array.at(tile.x, tile.y).corners[0] = type;
		tile_array.at(tile.x, tile.y).corners[1] = type;
	}
	// bottom right
	tile.x++;
	if (!is_out_of_bounds(tile)) {
		tile_array.at(tile.x, tile.y).corners[0] = type;
	}
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

void world_terrain::load(const std::string& path) {
	stream.set_read_index(0);
	no::file::read(path, stream);
	tile_array.read(stream, 1024, {});
	dirty = true;
}

void world_terrain::save(const std::string& path) const {
	tile_array.write(stream, 1024, {});
	stream.set_write_index(stream.size());
	no::file::write(path, stream);
}

void world_terrain::shift_left() {
	tile_array.shift_left(stream, 1024, {});
	dirty = true;
}

void world_terrain::shift_right() {
	tile_array.shift_right(stream, 1024, {});
	dirty = true;
}

void world_terrain::shift_up() {
	tile_array.shift_up(stream, 1024, {});
	dirty = true;
}

void world_terrain::shift_down() {
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

world_state::world_state() : terrain(*this), objects(*this) {
	
}

void world_state::update() {
	objects.update();
}

no::vector2i world_state::world_position_to_tile_index(float x, float z) const {
	return { (int)std::floor(x), (int)std::floor(z) };
}

no::vector3f world_state::tile_index_to_world_position(int x, int z) const {
	return { (float)x, 0.0f, (float)z };
}

void world_state::load(const std::string& path) {
	terrain.load(path + "t");
	objects.load(path + "o");
}

void world_state::save(const std::string& path) const {
	terrain.save(path + "t");
	objects.save(path + "o");
}
