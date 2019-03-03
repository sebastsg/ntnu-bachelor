#pragma once

#include "world.hpp"
#include "draw.hpp"
#include "character.hpp"
#include "decoration.hpp"

class world_view;

struct static_object_vertex {
	static constexpr no::vertex_attribute_specification attributes[] = { 3, 3, 2 };
	no::vector3f position;
	no::vector3f normal;
	no::vector2f tex_coords;
};

class character_renderer {
public:

	character_renderer(world_view& world);
	character_renderer(const character_renderer&) = delete;
	character_renderer(character_renderer&&) = delete;

	~character_renderer();

	character_renderer& operator=(const character_renderer&) = delete;
	character_renderer& operator=(character_renderer&&) = delete;

	void draw();

	void add(character_object* object);
	void remove(character_object* object);

private:

	struct object_data {
		character_object* object = nullptr;
		no::model_instance model;
		int equip_event = -1;
		std::unordered_map<equipment_slot, int> attachments;

		object_data() = default;
		object_data(character_object* object, no::model& model) : object(object), model(model) {}
	};

	no::model model;
	std::unordered_map<long long, no::model*> equipments;

	int player_texture = -1;

	int idle = 0;
	int run = 0;

	std::vector<object_data> characters;

	world_view& world;

};

class decoration_renderer {
public:

	decoration_renderer();
	decoration_renderer(const decoration_renderer&) = delete;
	decoration_renderer(decoration_renderer&&) = delete;

	~decoration_renderer();

	decoration_renderer& operator=(const decoration_renderer&) = delete;
	decoration_renderer& operator=(decoration_renderer&&) = delete;

	void draw();

	void add(decoration_object* object);
	void remove(decoration_object* object);

private:

	struct object_group {
		int definition_id = -1;
		int texture = -1;
		no::model model;
		std::vector<decoration_object*> objects;
	};

	std::vector<object_group> groups;

};

class object_pick_renderer {
public:

	no::shader_variable var_pick_color;

	object_pick_renderer();
	object_pick_renderer(const object_pick_renderer&) = delete;
	object_pick_renderer(object_pick_renderer&&) = delete;

	~object_pick_renderer();

	object_pick_renderer& operator=(const object_pick_renderer&) = delete;
	object_pick_renderer& operator=(object_pick_renderer&&) = delete;

	void draw();

	void add(game_object* object);
	void remove(game_object* object);

private:

	std::vector<game_object*> objects;
	no::model box;

};

class world_view {
public:

	struct {
		float start = 30.0f;
		float distance = 70.0f;
		no::shader_variable var_start;
		no::shader_variable var_distance;
	} fog;

	struct {
		no::vector3f position;
		no::vector3f color = 1.0f;
		no::shader_variable var_position_animate;
		no::shader_variable var_color_animate;
		no::shader_variable var_position_static;
		no::shader_variable var_color_static;
	} light;

	no::shader_variable var_color;
	no::shader_variable var_pick_color;

	no::perspective_camera camera;
	no::model_attachment_mapping_list mappings;

	world_view(world_state& world);
	world_view(const world_view&) = delete;
	world_view(world_view&&) = delete;

	~world_view();

	world_view& operator=(const world_view&) = delete;
	world_view& operator=(world_view&&) = delete;

	void draw();
	void draw_terrain();
	void draw_for_picking();
	void draw_tile_highlights(const std::vector<no::vector2i>& tiles, const no::vector4f& color);

	void refresh_terrain();

	void add(game_object* object);
	void remove(game_object* object);

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
	decoration_renderer decorations;
	character_renderer characters;
	object_pick_renderer pick_objects;

	int animate_diffuse_shader = -1;
	int static_diffuse_shader = -1;
	int pick_shader = -1;

	no::tiled_quad_array<static_object_vertex> height_map;
	no::tiled_quad_array<no::pick_vertex> height_map_pick;

	no::quad<static_object_vertex> highlight_quad;
	int highlight_texture = -1;

	int add_object_id = -1;
	int remove_object_id = -1;

};
