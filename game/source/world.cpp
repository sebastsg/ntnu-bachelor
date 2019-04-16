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

bool world_tile::is_water() const {
	return flag(water_flag);
}

void world_tile::set_water(bool water) {
	set_flag(water_flag, water);
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
	row++;
	row += 2;
	add_group(world_tile::grass, world_tile::dirt);
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
	
}

bool world_terrain::is_out_of_bounds(no::vector2i tile) const {
	if (chunks[top_left].offset.x > tile.x || chunks[top_left].offset.y > tile.y) {
		return true;
	}
	if (chunks[bottom_right].offset.x + world_tile_chunk::width <= tile.x) {
		return true;
	}
	if (chunks[bottom_right].offset.y + world_tile_chunk::width <= tile.y) {
		return true;
	}
	return false;
}

float world_terrain::elevation_at(no::vector2i tile) const {
	if (is_out_of_bounds(tile)) {
		return 0.0f;
	}
	return tile_at(tile).height;
}

float world_terrain::average_elevation_at(no::vector2i tile) const {
	if (is_out_of_bounds(tile) || is_out_of_bounds(tile + 1)) {
		return 0.0f;
	}
	float sum = tile_at({ tile.x, tile.y }).height;
	sum += tile_at({ tile.x + 1, tile.y }).height;
	sum += tile_at({ tile.x + 1, tile.y + 1 }).height;
	sum += tile_at({ tile.x, tile.y + 1 }).height;
	return sum / 4.0f;
}

void world_terrain::set_elevation_at(no::vector2i tile, float elevation) {
	if (is_out_of_bounds(tile) || is_out_of_bounds(tile + 1)) {
		return;
	}
	tile_at(tile).height = elevation;
	chunk_at_tile(tile).dirty = true;
}

void world_terrain::elevate_tile(no::vector2i tile, float amount) {
	if (is_out_of_bounds(tile) || is_out_of_bounds(tile + 1)) {
		return;
	}
	tile_at(tile).height += amount;
	tile_at({ tile.x + 1, tile.y }).height += amount;
	tile_at({ tile.x, tile.y + 1 }).height += amount;
	tile_at({ tile.x + 1, tile.y + 1 }).height += amount;
	chunk_at_tile(tile).dirty = true;
}

void world_terrain::set_tile_type(no::vector2i tile, int type) {
	// top left
	tile -= 1;
	if (!is_out_of_bounds(tile)) {
		tile_at(tile).set_corner(3, type);
	}
	// top middle
	tile.x++;
	if (!is_out_of_bounds(tile)) {
		tile_at(tile).set_corner(2, type);
		tile_at(tile).set_corner(3, type);
	}
	// top right
	tile.x++;
	if (!is_out_of_bounds(tile)) {
		tile_at(tile).set_corner(2, type);
	}
	// middle left
	tile.x -= 2;
	tile.y++;
	if (!is_out_of_bounds(tile)) {
		tile_at(tile).set_corner(1, type);
		tile_at(tile).set_corner(3, type);
	}
	// middle
	tile.x++;
	if (!is_out_of_bounds(tile)) {
		tile_at(tile).set(type);
	}
	// middle right
	tile.x++;
	if (!is_out_of_bounds(tile)) {
		tile_at(tile).set_corner(0, type);
		tile_at(tile).set_corner(2, type);
	}
	// bottom left
	tile.x -= 2;
	tile.y++;
	if (!is_out_of_bounds(tile)) {
		tile_at(tile).set_corner(1, type);
	}
	// bottom middle
	tile.x++;
	if (!is_out_of_bounds(tile)) {
		tile_at(tile).set_corner(0, type);
		tile_at(tile).set_corner(1, type);
	}
	// bottom right
	tile.x++;
	if (!is_out_of_bounds(tile)) {
		tile_at(tile).set_corner(0, type);
	}
	chunk_at_tile(tile).dirty = true;
}

void world_terrain::set_tile_solid(no::vector2i tile, bool solid) {
	set_tile_flag(tile, world_tile::solid_flag, solid);
}

void world_terrain::set_tile_water(no::vector2i tile, bool water) {
	set_tile_flag(tile, world_tile::water_flag, water);
}

void world_terrain::set_tile_flag(no::vector2i tile, int flag, bool value) {
	if (!is_out_of_bounds(tile)) {
		tile_at(tile).set_flag(flag, value);
		chunk_at_tile(tile).dirty = true;
	}
}

no::vector2i world_terrain::offset() const {
	return chunks[top_left].offset;
}

world_tile& world_terrain::tile_at(no::vector2i tile) {
	int x = (tile.x - chunks[top_left].offset.x) / world_tile_chunk::width;
	int y = (tile.y - chunks[top_left].offset.y) / world_tile_chunk::width;
	auto& chunk = chunks[y * 3 + x];
	x = tile.x - chunk.offset.x;
	y = tile.y - chunk.offset.y;
	return chunk.tiles[y * world_tile_chunk::width + x];
}

const world_tile& world_terrain::tile_at(no::vector2i tile) const {
	int x = (tile.x - chunks[top_left].offset.x) / world_tile_chunk::width;
	int y = (tile.y - chunks[top_left].offset.y) / world_tile_chunk::width;
	auto & chunk = chunks[y * 3 + x];
	x = tile.x - chunk.offset.x;
	y = tile.y - chunk.offset.y;
	return chunk.tiles[y * world_tile_chunk::width + x];
}

world_tile& world_terrain::local_tile_at(no::vector2i tile) {
	int x = tile.x / world_tile_chunk::width;
	int y = tile.y / world_tile_chunk::width;
	auto& chunk = chunks[y * 3 + x];
	tile.x -= x * world_tile_chunk::width;
	tile.y -= y * world_tile_chunk::width;
	return chunk.tiles[tile.y * world_tile_chunk::width + tile.x];
}

void world_terrain::load_chunk(no::vector2i chunk_index, int index) {
	no::io_stream stream;
	no::file::read(chunk_path(chunk_index), stream);
	chunks[index].dirty = true;
	chunks[index].offset = chunk_index * world_tile_chunk::width;
	if (stream.size_left_to_read() > 0) {
		for (int i = 0; i < world_tile_chunk::total; i++) {
			auto& tile = chunks[index].tiles[i];
			tile.height = stream.read<float>();
			tile.water_height = stream.read<float>();
			tile.corners[0] = stream.read<uint8_t>();
			tile.corners[1] = stream.read<uint8_t>();
			tile.corners[2] = stream.read<uint8_t>();
			tile.corners[3] = stream.read<uint8_t>();
		}
	} else {
		for (int i = 0; i < world_tile_chunk::total; i++) {
			chunks[index].tiles[i] = {};
		}
	}
}

void world_terrain::save_chunk(no::vector2i chunk_index, int index) const {
	no::io_stream stream;
	for (auto& tile : chunks[index].tiles) {
		stream.write(tile.height);
		stream.write(tile.water_height);
		stream.write(tile.corners[0]);
		stream.write(tile.corners[1]);
		stream.write(tile.corners[2]);
		stream.write(tile.corners[3]);
	}
	no::file::write(chunk_path(chunk_index), stream);
}

void world_terrain::load(no::vector2i center) {
	center.x = std::max(1, center.x);
	center.y = std::max(1, center.y);
	load_chunk(center - 1, top_left);
	load_chunk({ center.x, center.y - 1 }, top_middle);
	load_chunk({ center.x + 1, center.y - 1 }, top_right);
	load_chunk({ center.x - 1, center.y }, middle_left);
	load_chunk(center, middle);
	load_chunk({ center.x + 1, center.y }, middle_right);
	load_chunk({ center.x - 1, center.y + 1 }, bottom_left);
	load_chunk({ center.x, center.y + 1 }, bottom_middle);
	load_chunk(center + 1, bottom_right);
}

void world_terrain::save() {
	for (int i = 0; i < 9; i++) {
		save_chunk(chunks[i].index(), i);
	}
}

void world_terrain::shift_left() {
	no::vector2i index = chunks[top_left].index();
	// swap right and middle columns
	std::swap(chunks[top_middle], chunks[top_right]);
	std::swap(chunks[middle], chunks[middle_right]);
	std::swap(chunks[bottom_middle], chunks[bottom_right]);
	// swap left and middle columns
	std::swap(chunks[top_left], chunks[top_middle]);
	std::swap(chunks[middle_left], chunks[middle]);
	std::swap(chunks[bottom_left], chunks[bottom_middle]);
	// overwrite left column
	index.x--;
	load_chunk(index, top_left);
	index.y++;
	load_chunk(index, middle_left);
	index.y++;
	load_chunk(index, bottom_left);
	for (int i = 0; i < 9; i++) {
		chunks[i].dirty = true;
	}
	events.shift_left.emit();
}

void world_terrain::shift_right() {
	no::vector2i index = chunks[top_right].index();
	// swap left and middle columns
	std::swap(chunks[top_middle], chunks[top_left]);
	std::swap(chunks[middle], chunks[middle_left]);
	std::swap(chunks[bottom_middle], chunks[bottom_left]);
	// swap right and middle columns
	std::swap(chunks[top_right], chunks[top_middle]);
	std::swap(chunks[middle_right], chunks[middle]);
	std::swap(chunks[bottom_right], chunks[bottom_middle]);
	// overwrite right column
	index.x++;
	load_chunk(index, top_right);
	index.y++;
	load_chunk(index, middle_right);
	index.y++;
	load_chunk(index, bottom_right);
	for (int i = 0; i < 9; i++) {
		chunks[i].dirty = true;
	}
	events.shift_right.emit();
}

void world_terrain::shift_up() {
	no::vector2i index = chunks[top_left].index();
	// swap bottom and middle rows
	std::swap(chunks[bottom_left], chunks[middle_left]);
	std::swap(chunks[bottom_middle], chunks[middle]);
	std::swap(chunks[bottom_right], chunks[middle_right]);
	// swap top and middle rows
	std::swap(chunks[top_left], chunks[middle_left]);
	std::swap(chunks[top_middle], chunks[middle]);
	std::swap(chunks[top_right], chunks[middle_right]);
	// overwrite top row
	index.y--;
	load_chunk(index, top_left);
	index.x++;
	load_chunk(index, top_middle);
	index.x++;
	load_chunk(index, top_right);
	for (int i = 0; i < 9; i++) {
		chunks[i].dirty = true;
	}
	events.shift_up.emit();
}

void world_terrain::shift_down() {
	no::vector2i index = chunks[bottom_left].index();
	// swap top and middle rows
	std::swap(chunks[top_left], chunks[middle_left]);
	std::swap(chunks[top_middle], chunks[middle]);
	std::swap(chunks[top_right], chunks[middle_right]);
	// swap bottom and middle rows
	std::swap(chunks[bottom_left], chunks[middle_left]);
	std::swap(chunks[bottom_middle], chunks[middle]);
	std::swap(chunks[bottom_right], chunks[middle_right]);
	// overwrite bottom row
	index.y++;
	load_chunk(index, bottom_left);
	index.x++;
	load_chunk(index, bottom_middle);
	index.x++;
	load_chunk(index, bottom_right);
	for (int i = 0; i < 9; i++) {
		chunks[i].dirty = true;
	}
	events.shift_down.emit();
}

void world_terrain::shift_to_center_of(no::vector2i tile) {
	no::vector2i size = world_tile_chunk::width * 3;
	no::vector2i current = offset();
	no::vector2i goal = tile - size / 2;
	current /= world_tile_chunk::width;
	goal /= world_tile_chunk::width;
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

void world_terrain::for_each_neighbour(no::vector2i index, const std::function<void(no::vector2i, const world_tile&)>& function) const {
	const int x = index.x;
	const int y = index.y;
	const no::vector2i offset = chunks[top_left].offset;
	const bool left = (x - 1 - offset.x >= 0);
	const bool right = (x + 1 - offset.x < size().x);
	const bool up = (y - 1 - offset.y >= 0);
	const bool down = (y + 1 - offset.y < size().y);
	if (left) {
		function({ x - 1, y }, tile_at({ x - 1, y }));
		if (up) {
			function({ x - 1, y - 1 }, tile_at({ x - 1, y - 1 }));
		}
		if (down) {
			function({ x - 1, y + 1 }, tile_at({ x - 1, y + 1 }));
		}
	}
	if (right) {
		function({ x + 1, y }, tile_at({ x + 1, y }));
		if (up) {
			function({ x + 1, y - 1 }, tile_at({ x + 1, y - 1 }));
		}
		if (down) {
			function({ x + 1, y + 1 }, tile_at({ x + 1, y + 1 }));
		}
	}
	if (up) {
		function({ x, y - 1 }, tile_at({ x, y - 1 }));
	}
	if (down) {
		function({ x, y + 1 }, tile_at({ x, y + 1 }));
	}
}

world_tile_chunk& world_terrain::chunk_at_tile(no::vector2i tile) {
	int x = (tile.x - chunks[top_left].offset.x) / world_tile_chunk::width;
	int y = (tile.y - chunks[top_left].offset.y) / world_tile_chunk::width;
	return chunks[y * 3 + x];
}

std::string world_terrain::chunk_path(no::vector2i index) const {
	return no::asset_path("worlds/" + world.name + "_" + std::to_string(index.x) + "_" + std::to_string(index.y) + ".ec");
}

world_state::world_state() : terrain(*this), objects(*this) {
	
}

void world_state::update() {
	objects.update();
}

std::vector<no::vector2i> world_state::path_between(no::vector2i from, no::vector2i to) const {
	return pathfinder{ terrain }.find_path(from, to);
}

bool world_state::can_fish_at(no::vector2i from, no::vector2i to) const {
	return terrain.tile_at(to).is_water() && from.distance_to(to) < 15;
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
