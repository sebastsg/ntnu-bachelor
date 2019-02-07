#pragma once

#include "object.hpp"
#include "player.hpp"

#include "math.hpp"
#include "camera.hpp"
#include "containers.hpp"

class world_state;
class player_object;
class decoration_object;
class game_object;

struct world_tile {
	float height = 0.0f;
	uint8_t corners[4] = {};
	void set(int type) {
		corners[0] = (uint8_t)type;
		corners[1] = (uint8_t)type;
		corners[2] = (uint8_t)type;
		corners[3] = (uint8_t)type;
	}
};

class world_autotiler {
public:

	static constexpr int grass = 0;
	static constexpr int dirt = 1;
	static constexpr int water = 2;

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

	no::vector2i offset() const;
	no::vector2i size() const;
	const no::shifting_2d_array<world_tile>& tiles() const;

	void read(no::io_stream& stream);
	void write(no::io_stream& stream) const;

	void shift_left(no::io_stream& stream);
	void shift_right(no::io_stream& stream);
	void shift_up(no::io_stream& stream);
	void shift_down(no::io_stream& stream);

	bool is_dirty() const;
	void set_clean();

private:

	no::vector2i global_to_local_tile(no::vector2i tile) const;
	no::vector2i local_to_global_tile(no::vector2i tile) const;

	world_state& world;
	no::shifting_2d_array<world_tile> tile_array;
	bool dirty = false;

};

class world_state {
public:

	world_terrain terrain;

	world_state();
	~world_state();

	void update();

	player_object* add_player(int id);
	player_object* player(int id);
	void remove_player(int id);

	decoration_object* add_decoration();
	void remove_decoration(decoration_object* decoration);

	no::vector2i world_position_to_tile_index(float x, float z) const;
	no::vector3f tile_index_to_world_position(int x, int z) const;

	int next_object_id();

	item_definition_list& items();

private:

	int object_id_counter = 0;
	std::vector<player_object*> players;
	std::vector<decoration_object*> decorations;
	item_definition_list item_definitions;

};
