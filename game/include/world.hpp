#pragma once

#include "object.hpp"

#include "math.hpp"
#include "camera.hpp"
#include "containers.hpp"

class world_state;

struct world_tile {

	static const size_t solid_flag = 0;
	static const size_t unused_1_flag = 1;
	static const size_t unused_2_flag = 2;
	static const size_t unused_3_flag = 3;

	// max is 127 - the leftmost bit is reserved for flags
	static const uint8_t grass = 0;
	static const uint8_t dirt = 1;
	static const uint8_t water = 2;

	float height = 0.0f;

	void set(uint8_t type);
	void set_corner(int index, uint8_t type);
	uint8_t corner(int index) const;

	bool is_solid() const;
	void set_solid(bool solid);

	bool flag(int index) const;
	void set_flag(int index, bool value);

private:

	uint8_t corners[4] = {};

	static const uint8_t flag_bits = 0b10000000;
	static const uint8_t tile_bits = 0b01111111;
	
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

class world_terrain {
public:

	world_autotiler autotiler;

	world_terrain(world_state& world);
	world_terrain(const world_terrain&) = delete;
	world_terrain(world_terrain&&) = default;

	world_terrain& operator=(const world_terrain&) = delete;
	world_terrain& operator=(world_terrain&&) = delete;

	bool is_out_of_bounds(no::vector2i tile) const;
	float elevation_at(no::vector2i tile) const;
	void set_elevation_at(no::vector2i tile, float elevation);
	void elevate_tile(no::vector2i tile, float amount);

	void set_tile_type(no::vector2i tile, int type);
	void set_tile_solid(no::vector2i tile, bool solid);
	void set_tile_flag(no::vector2i tile, int flag, bool value);

	no::vector2i offset() const;
	no::vector2i size() const;
	const no::shifting_2d_array<world_tile>& tiles() const;

	void load(const std::string& path);
	void save(const std::string& path) const;

	void shift_left();
	void shift_right();
	void shift_up();
	void shift_down();

	bool is_dirty() const;
	void set_clean();

private:

	no::vector2i global_to_local_tile(no::vector2i tile) const;
	no::vector2i local_to_global_tile(no::vector2i tile) const;

	world_state& world;
	no::shifting_2d_array<world_tile> tile_array;
	bool dirty = false;

	mutable no::io_stream stream;

};

class world_state {
public:

	world_terrain terrain;
	world_objects objects;

	world_state();
	world_state(const world_state&) = delete;
	world_state(world_state&&) = delete;
	
	virtual ~world_state() = default;

	world_state& operator=(const world_state&) = delete;
	world_state& operator=(world_state&&) = delete;

	virtual void update();

	no::vector2i world_position_to_tile_index(float x, float z) const;
	no::vector3f tile_index_to_world_position(int x, int z) const;

	void load(const std::string& path);
	void save(const std::string& path) const;

};
