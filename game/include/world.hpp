#pragma once

#include "math.hpp"
#include "draw.hpp"
#include "camera.hpp"
#include "player.hpp"

#include <unordered_map>

class world_state;

class world_terrain {
public:

	no::shifting_2d_array<float> tile_heights;

	world_terrain(world_state& world);
	world_terrain(const world_terrain&) = delete;
	world_terrain(world_terrain&&) = default;

	world_terrain& operator=(const world_terrain&) = delete;
	world_terrain& operator=(world_terrain&&) = delete;

	bool is_out_of_bounds(no::vector2i tile) const;
	float elevation_at(no::vector2i tile) const;
	void set_elevation_at(no::vector2i tile, float elevation);
	void elevate_tile(no::vector2i tile, float amount);

private:

	no::vector2i global_to_local_tile(no::vector2i tile) const;
	no::vector2i local_to_global_tile(no::vector2i tile) const;

	world_state& world;

};

class world_state {
public:

	int my_player_id = -1;

	no::perspective_camera camera;

	world_state();

	player_object& add_player(int id);

	player_object& player(int id);
	player_object& my_player();
	const player_object& my_player() const;

	no::vector2i world_position_to_tile_index(float x, float z) const;
	no::vector3f tile_index_to_world_position(int x, int z) const;

	std::unordered_map<int, player_object> players;

	world_terrain terrain;

};

class world_view {
public:

	world_view(world_state& world);

	void draw();
	void draw_for_picking();
	void draw_tile_highlight(no::vector2i tile);

	void refresh_terrain();

private:

	world_state& world;

	void draw_terrain();
	void draw_players();

	int grass_texture = -1;
	int heightmap_shader = -1;
	int diffuse_shader = -1;
	int basic_mesh_shader = -1;
	int blank_texture = -1;
	int pick_shader = -1;
	
	player_renderer player_render;
	
	no::tiled_quad_array<no::height_map_vertex> height_map;
	no::tiled_quad_array<no::pick_vertex> height_map_pick;

	no::quad<no::static_mesh_vertex> highlight_quad;
	
};