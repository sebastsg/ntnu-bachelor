#pragma once

#include "object.hpp"
#include "world_objects.hpp"

#include "math.hpp"
#include "camera.hpp"
#include "containers.hpp"

class world_state;

struct world_tile {

	static const uint8_t flag_bits = 0b11000000;
	static const uint8_t tile_bits = 0b00111111;

	static const size_t solid_flag = 0;
	static const size_t water_flag = 1;
	static const size_t unused_2_flag = 2;
	static const size_t unused_3_flag = 3;

	// max is 64 - the two leftmost bits are reserved for flags
	static const uint8_t grass = 0;
	static const uint8_t dirt = 1;
	static const uint8_t stone = 2;

	float height = 0.0f;
	float water_height = 0.0f;
	uint8_t corners[4] = {};

	void set(uint8_t type);
	void set_corner(int index, uint8_t type);
	uint8_t corner(int index) const;
	int corners_of_type(uint8_t type) const;

	bool is_solid() const;
	void set_solid(bool solid);

	bool is_water() const;
	void set_water(bool water);

	bool flag(int index) const;
	void set_flag(int index, bool value);

	float pick_height() const;

};

class world_autotiler {
public:

	world_autotiler();

	uint32_t packed_corners(int top_left, int top_right, int bottom_left, int bottom_right) const;
	no::vector2i uv_index(uint32_t corners) const;

private:

	void add_main(int tile);
	void add_group(int tile, int bordering_tile);

	std::unordered_map<uint32_t, no::vector2i> uv_indices;
	int row = 0;

};

struct water_area {
	no::vector2i position;
	no::vector2i size;
	float height = 0.0f;
};

struct world_tile_chunk {

	static const int width = 64;
	static const int total = width * width;

	no::vector2i offset;
	world_tile tiles[total];
	std::vector<water_area> water_areas;
	bool dirty = false;

	inline no::vector2i index() const {
		return offset / width;
	}

};

class world_terrain {
public:

	static const int top_left = 0;
	static const int top_middle = 1;
	static const int top_right = 2;
	static const int middle_left = 3;
	static const int middle = 4;
	static const int middle_right = 5;
	static const int bottom_left = 6;
	static const int bottom_middle = 7;
	static const int bottom_right = 8;

	world_tile_chunk chunks[9];
	world_autotiler autotiler;
	
	struct {
		no::signal_event shift_left;
		no::signal_event shift_right;
		no::signal_event shift_up;
		no::signal_event shift_down;
	} events;

	world_terrain(world_state& world);
	world_terrain(const world_terrain&) = delete;
	world_terrain(world_terrain&&) = delete;

	world_terrain& operator=(const world_terrain&) = delete;
	world_terrain& operator=(world_terrain&&) = delete;

	bool is_out_of_bounds(no::vector2i tile) const;
	float elevation_at(no::vector2i tile) const;
	float local_elevation_at(no::vector2i tile) const;
	float pick_elevation_at(no::vector2i tile) const;
	void set_elevation_at(no::vector2i tile, float elevation);
	void elevate_tile(no::vector2i tile, float amount);
	float average_elevation_at(no::vector2i tile) const;

	void set_tile_type(no::vector2i tile, int type);
	void set_tile_solid(no::vector2i tile, bool solid);
	void set_tile_water(no::vector2i tile, bool water);
	void set_tile_flag(no::vector2i tile, int flag, bool value);

	no::vector2i offset() const;
	world_tile& tile_at(no::vector2i tile);
	const world_tile& tile_at(no::vector2i tile) const;
	world_tile& local_tile_at(no::vector2i tile);
	const world_tile& local_tile_at(no::vector2i tile) const;

	no::vector3f calculate_normal(no::vector2i tile, int corner) const; // vertex normal
	no::vector3f calculate_normal(no::vector2i tile) const; // face normal

	void load_chunk(no::vector2i chunk_index, int index);
	void save_chunk(no::vector2i chunk_index, int index) const;
	void load(no::vector2i center_chunk);
	void save();

	void shift_left();
	void shift_right();
	void shift_up();
	void shift_down();
	void shift_to_center_of(no::vector2i tile);

	void for_each_neighbour(no::vector2i index, const std::function<void(no::vector2i, const world_tile&)>& function) const;
	world_tile_chunk& chunk_at_tile(no::vector2i tile);

	no::vector2i size() const {
		return world_tile_chunk::width * 3;
	}

private:

	std::string chunk_path(no::vector2i index) const;

	world_state& world;

};

class world_state {
public:

	world_terrain terrain;
	world_objects objects;
	std::string name;

	world_state();
	world_state(const world_state&) = delete;
	world_state(world_state&&) = delete;
	
	virtual ~world_state() = default;

	world_state& operator=(const world_state&) = delete;
	world_state& operator=(world_state&&) = delete;

	virtual void update();

	std::vector<no::vector2i> path_between(no::vector2i from, no::vector2i to) const;
	bool can_fish_at(no::vector2i from, no::vector2i to) const;

};

no::vector2i world_position_to_tile_index(float x, float z);
no::vector2i world_position_to_tile_index(const no::vector3f& position);
no::vector3f tile_index_to_world_position(int x, int z);
no::vector3f tile_index_to_world_position(no::vector2i tile);
