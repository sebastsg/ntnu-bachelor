#pragma once

#include "world.hpp"
#include "skeletal.hpp"
#include "character.hpp"
#include "world_objects.hpp"

class world_view;

struct static_object_vertex {
	static constexpr no::vertex_attribute_specification attributes[] = { 3, 3, 2 };
	no::vector3f position;
	no::vector3f normal;
	no::vector2f tex_coords;
};

class character_renderer {
public:

	struct {
		no::shader_variable bones;
	} shader;

	character_renderer();
	character_renderer(const character_renderer&) = delete;
	character_renderer(character_renderer&&) = delete;

	~character_renderer();

	character_renderer& operator=(const character_renderer&) = delete;
	character_renderer& operator=(character_renderer&&) = delete;

	void update(const no::bone_attachment_mapping_list& mappings, const world_objects& objects);
	void draw();
	void add(world_objects& objects, int object_id);
	void remove(character_object& object);

private:

	struct character_events {
		int equip = -1;
		int unequip = -1;
		int attack = -1;
		int defend = -1;
		int idle = -1;
		int move = -1;
		int start_fishing = -1;
		int stop_fishing = -1;
	};

	struct character_equipment {
		int animation_id = -1;
		int item_id = -1;
	};

	struct character_animation {
		int object_id = -1;
		int animation_id = -1;
		std::string model;
		std::string animation;
		character_events events;
		std::vector<character_equipment> equipments;
		bool new_animation = true;
		bool play_once = false;
		bool showing_legs = true;
		bool showing_body = true;
	};

	struct textured_model {
		no::model model;
		int texture = -1;
	};

	void on_equip(character_animation& object, const item_instance& item);
	void on_unequip(character_animation& object, equipment_slot slot);

	// these functions are kind of a hack, and only apply to the character model
	void hide_legs(character_animation& object);
	void hide_body(character_animation& object);
	void show_legs(character_animation& object);
	void show_body(character_animation& object);
	void filter_character_model();

	std::thread thread;
	std::atomic<bool> joining_thread = false;

	std::vector<character_animation> characters;

	std::unordered_map<std::string, textured_model> character_models;
	std::unordered_map<int, textured_model> equipment_models;

	std::unordered_map<std::string, no::skeletal_animator> animators;
	std::unordered_map<int, no::skeletal_animator> equipment_animators;

};

class decoration_renderer {
public:

	decoration_renderer();
	decoration_renderer(const decoration_renderer&) = delete;
	decoration_renderer(decoration_renderer&&) = delete;

	~decoration_renderer();

	decoration_renderer& operator=(const decoration_renderer&) = delete;
	decoration_renderer& operator=(decoration_renderer&&) = delete;

	void draw(const world_objects& objects);
	void add(const game_object& object);
	void remove(const game_object& object);

private:

	struct object_group {
		int definition_id = -1;
		int texture = -1;
		no::model model;
		std::vector<int> objects;
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

	void draw(no::vector2i offset, const world_objects& objects);
	void add(const game_object& object);
	void remove(const game_object& object);

private:

	std::vector<int> object_ids;
	no::model box;

};

struct water_vertex {
	static constexpr no::vertex_attribute_specification attributes[] = { 3 };
	no::vector3f position;
};

class world_view {
public:

	struct {
		no::vector3f position;
		no::vector3f color = 1.0f;
		no::shader_variable var_position_animate;
		no::shader_variable var_color_animate;
		no::shader_variable var_position_static;
		no::shader_variable var_color_static;
		no::shader_variable var_position_water;
		no::shader_variable var_color_water;
	} light;

	no::shader_variable var_color;
	no::shader_variable var_pick_color;

	no::perspective_camera camera;
	no::bone_attachment_mapping_list mappings;

	world_view(world_state& world);
	world_view(const world_view&) = delete;
	world_view(world_view&&) = delete;

	~world_view();

	world_view& operator=(const world_view&) = delete;
	world_view& operator=(world_view&&) = delete;

	void draw();
	void draw_terrain();
	void draw_water();
	void draw_for_picking();
	void draw_tile_highlights(const std::vector<no::vector2i>& tiles, const no::vector4f& color);
	void refresh_terrain();
	void update_object_visibility();
	void reload_shaders();

private:

	void add(const game_object& object);
	void remove(const game_object& object);

	void build_chunk(int chunk);
	void refresh_chunk(int chunk);
	void swap_chunks(int first, int second);

	struct {
		int grid = 52;
		int border = 34;
		int per_row = 10;
		int per_column = 20;
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
	std::vector<bool> object_visibilities;

	int animate_diffuse_shader = -1;
	int static_diffuse_shader = -1;
	int pick_shader = -1;
	int water_shader = -1;

	no::tiled_quad_array<static_object_vertex> height_map[9];
	no::tiled_quad_array<no::pick_vertex> height_map_pick[9];
	no::quad<water_vertex> water_quad;

	int shift_left_event = -1;
	int shift_right_event = -1;
	int shift_up_event = -1;
	int shift_down_event = -1;

	no::quad<static_object_vertex> highlight_quad;
	int highlight_texture = -1;

	int add_object_id = -1;
	int remove_object_id = -1;

};
