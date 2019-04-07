#include "world.hpp"
#include "assets.hpp"
#include "pathfinding.hpp"

void world_tile::set(uint8_t type) {
	set_corner(0, type);
	set_corner(1, type);
	set_corner(2, type);
	set_corner(3, type);
}

void world_tile::set_corner(int index, uint8_t type) {
	corners[index] = type | (corners[index] & flag_bits);
}

uint8_t world_tile::corner(int index) const {
	return corners[index] & tile_bits;
}

int world_tile::corners_of_type(uint8_t type) const {
	return (int)(corner(0) == type) + (int)(corner(1) == type) + (int)(corner(2) == type) + (int)(corner(3) == type);
}

bool world_tile::is_solid() const {
	return flag(solid_flag);
}

void world_tile::set_solid(bool solid) {
	set_flag(solid_flag, solid);
}

bool world_tile::flag(int index) const {
	return corners[index] & flag_bits;
}

void world_tile::set_flag(int index, bool value) {
	corners[index] = corner(index) | (value ? flag_bits : 0);
}

world_autotiler::world_autotiler() {
	add_main(world_tile::grass);
	add_main(world_tile::dirt);
	add_main(world_tile::water);
	row++;
	row += 2;
	add_group(world_tile::grass, world_tile::dirt);
	add_group(world_tile::grass, world_tile::water);
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
	tile_array.resize_and_reset(active_radius, {});
}

bool world_terrain::is_out_of_bounds(no::vector2i tile) const {
	return tile_array.is_out_of_bounds(tile.x, tile.y);
}

float world_terrain::elevation_at(no::vector2i tile) const {
	return tile_array.at(tile.x, tile.y).height;
}

float world_terrain::average_elevation_at(no::vector2i tile) const {
	if (is_out_of_bounds(tile) || is_out_of_bounds(tile + 1)) {
		return 0.0f;
	}
	float sum = tile_array.at(tile.x, tile.y).height;
	sum += tile_array.at(tile.x + 1, tile.y).height;
	sum += tile_array.at(tile.x + 1, tile.y + 1).height;
	sum += tile_array.at(tile.x, tile.y + 1).height;
	return sum / 4.0f;
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
		tile_array.at(tile.x, tile.y).set_corner(3, type);
	}
	// top middle
	tile.x++;
	if (!is_out_of_bounds(tile)) {
		tile_array.at(tile.x, tile.y).set_corner(2, type);
		tile_array.at(tile.x, tile.y).set_corner(3, type);
	}
	// top right
	tile.x++;
	if (!is_out_of_bounds(tile)) {
		tile_array.at(tile.x, tile.y).set_corner(2, type);
	}
	// middle left
	tile.x -= 2;
	tile.y++;
	if (!is_out_of_bounds(tile)) {
		tile_array.at(tile.x, tile.y).set_corner(1, type);
		tile_array.at(tile.x, tile.y).set_corner(3, type);
	}
	// middle
	tile.x++;
	if (!is_out_of_bounds(tile)) {
		tile_array.at(tile.x, tile.y).set(type);
	}
	// middle right
	tile.x++;
	if (!is_out_of_bounds(tile)) {
		tile_array.at(tile.x, tile.y).set_corner(0, type);
		tile_array.at(tile.x, tile.y).set_corner(2, type);
	}
	// bottom left
	tile.x -= 2;
	tile.y++;
	if (!is_out_of_bounds(tile)) {
		tile_array.at(tile.x, tile.y).set_corner(1, type);
	}
	// bottom middle
	tile.x++;
	if (!is_out_of_bounds(tile)) {
		tile_array.at(tile.x, tile.y).set_corner(0, type);
		tile_array.at(tile.x, tile.y).set_corner(1, type);
	}
	// bottom right
	tile.x++;
	if (!is_out_of_bounds(tile)) {
		tile_array.at(tile.x, tile.y).set_corner(0, type);
	}
	dirty = true;
}

void world_terrain::set_tile_solid(no::vector2i tile, bool solid) {
	if (!is_out_of_bounds(tile)) {
		tile_array.at(tile.x, tile.y).set_solid(solid);
	}
}

void world_terrain::set_tile_flag(no::vector2i tile, int flag, bool value) {
	if (!is_out_of_bounds(tile)) {
		tile_array.at(tile.x, tile.y).set_flag(flag, value);
	}
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
	tile_array.read(stream, tile_stride, {});
	dirty = true;
}

void world_terrain::save(const std::string& path) const {
	tile_array.write(stream, tile_stride, {});
	stream.set_write_index(stream.size());
	no::file::write(path, stream);
}

void world_terrain::shift_left() {
	tile_array.shift_left(stream, tile_stride, {});
	dirty = true;
}

void world_terrain::shift_right() {
	tile_array.shift_right(stream, tile_stride, {});
	dirty = true;
}

void world_terrain::shift_up() {
	tile_array.shift_up(stream, tile_stride, {});
	dirty = true;
}

void world_terrain::shift_down() {
	tile_array.shift_down(stream, tile_stride, {});
	dirty = true;
}

void world_terrain::shift_to_center_of(no::vector2i tile) {
	no::vector2i current = offset();
	no::vector2i goal = tile - size() / 2;
	goal.x = std::max(0, goal.x);
	goal.y = std::max(0, goal.y);
	while (current.x > goal.x) {
		current.x--;
		shift_left();
	}
	while (current.x < goal.x) {
		current.x++;
		shift_right();
	}
	while (current.y > goal.y) {
		current.y--;
		shift_up();
	}
	while (current.y < goal.y) {
		current.y++;
		shift_down();
	}
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

void world_state::load(const std::string& path) {
	terrain.load(path + "t");
	objects.load(path + "o");
}

void world_state::save(const std::string& path) const {
	terrain.save(path + "t");
	objects.save(path + "o");
}

std::vector<no::vector2i> world_state::path_between(no::vector2i from, no::vector2i to) const {
	return pathfinder{ terrain }.find_path(from, to);
}

bool world_state::can_fish_at(no::vector2i from, no::vector2i to) const {
	return terrain.tiles().at(to.x, to.y).corners_of_type(world_tile::water) == 4 && from.distance_to(to) < 15;
}

no::vector2i world_position_to_tile_index(float x, float z) {
	return { (int)std::floor(x), (int)std::floor(z) };
}

no::vector2i world_position_to_tile_index(const no::vector3f& position) {
	return { (int)std::floor(position.x), (int)std::floor(position.z) };
}

no::vector3f tile_index_to_world_position(int x, int z) {
	return { (float)x + 0.5f, 0.0f, (float)z + 0.5f };
}

no::vector3f tile_index_to_world_position(no::vector2i tile) {
	return { (float)tile.x + 0.5f, 0.0f, (float)tile.y + 0.5f };
}
