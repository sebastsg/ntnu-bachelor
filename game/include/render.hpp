#pragma once

#include "world.hpp"
#include "draw.hpp"

class player_renderer {
public:

	player_renderer();

	void draw();

	void add(player_object* object);
	void remove(player_object* object);

private:

	struct object_data {
		player_object* object = nullptr;
		no::model_instance model;
		int equip_event = -1;
		int right_hand_attachment = -1;

		object_data() = default;
		object_data(player_object* object, no::model& model) : object(object), model(model) {}
	};

	no::model model;
	no::model sword_model;

	int idle = 0;
	int run = 0;

	std::vector<object_data> players;

};

class decoration_renderer {
public:

	decoration_renderer();

	void draw();

	void add(decoration_object* object);
	void remove(decoration_object* object);

private:

	struct object_data {
		decoration_object* object = nullptr;
		no::model model; // todo: use model_instance, but not really big deal atm
		object_data() = default;
		object_data(decoration_object* object);
	};

	// todo: decorations should be able to have different textures
	int texture = -1;

	std::vector<object_data> decorations;

};

class world_view {
public:

	no::perspective_camera camera;

	player_renderer players;
	decoration_renderer decorations;

	world_view(world_state& world);

	void draw();
	void draw_for_picking();
	void draw_tile_highlight(no::vector2i tile);

	void refresh_terrain();

private:

	world_state& world;

	void draw_terrain();
	void draw_players();
	void draw_decorations();

	int tiles_texture = -1;
	int diffuse_shader = -1;
	int static_textured_shader = -1;
	int pick_shader = -1;

	no::tiled_quad_array<no::static_textured_vertex> height_map;
	no::tiled_quad_array<no::pick_vertex> height_map_pick;

	no::quad<no::static_textured_vertex> highlight_quad;
	int highlight_texture = -1;

};
