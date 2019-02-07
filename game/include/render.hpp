#pragma once

#include "world.hpp"
#include "draw.hpp"

class world_view;

class player_renderer {
public:

	player_renderer(world_view& world);
	player_renderer(const player_renderer&) = delete;
	player_renderer(player_renderer&&) = delete;

	~player_renderer();

	player_renderer& operator=(const player_renderer&) = delete;
	player_renderer& operator=(player_renderer&&) = delete;

	void draw();

	void add(player_object* object);
	void remove(player_object* object);

private:

	struct object_data {
		player_object* object = nullptr;
		no::model_instance model;
		int equip_event = -1;
		std::unordered_map<equipment_slot, int> attachments;

		object_data() = default;
		object_data(player_object* object, no::model& model) : object(object), model(model) {}
	};

	no::model model;
	std::unordered_map<long long, no::model*> equipments;

	int player_texture = -1;

	int idle = 0;
	int run = 0;

	std::vector<object_data> players;

	world_view& world;

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

struct terrain_vertex {
	static constexpr no::vertex_attribute_specification attributes[] = { 3, 3, 2 };
	no::vector3f position;
	no::vector3f normal;
	no::vector2f tex_coords;
};

class world_view {
public:

	struct {
		float start = 30.0f;
		float distance = 70.0f;
		no::shader_variable var_start;
		no::shader_variable var_distance;
	} fog;

	no::perspective_camera camera;
	no::model_attachment_mapping_list mappings;

	player_renderer players;
	decoration_renderer decorations;

	world_view(world_state& world);

	void draw();
	void draw_for_picking();
	void draw_tile_highlight(no::vector2i tile);

	void refresh_terrain();

private:
	
	struct {
		int grid = 52;
		int border = 24;
		int per_row = 9;
		int per_column = 9;
		no::vector2i size;
		int texture = -1;
	} tileset;

	no::vector2f uv_step() const;
	no::vector2f uv_for_type(no::vector2i index) const;
	no::surface add_tile_borders(uint32_t* pixels, int width, int height);
	void repeat_tile_under_row(uint32_t* pixels, int width, int height, int tile, int new_row, int row);

	world_state& world;

	void draw_terrain();
	void draw_players();
	void draw_decorations();

	int diffuse_shader = -1;
	int static_textured_shader = -1;
	int pick_shader = -1;
	int terrain_shader = -1;

	no::tiled_quad_array<terrain_vertex> height_map;
	no::tiled_quad_array<no::pick_vertex> height_map_pick;

	no::quad<no::static_textured_vertex> highlight_quad;
	int highlight_texture = -1;

};
