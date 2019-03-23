#pragma once

#include "world.hpp"
#include "draw.hpp"
#include "font.hpp"
#include "character.hpp"
#include "ui.hpp"
#include "minimap.hpp"

class game_state;

class hit_splat {
public:

	hit_splat(game_state& game, int target_id, int value);
	hit_splat(const hit_splat&) = delete;
	hit_splat(hit_splat&&);

	~hit_splat();

	hit_splat& operator=(const hit_splat&) = delete;
	hit_splat& operator=(hit_splat&&);

	void update(const no::ortho_camera& camera);
	void draw(no::shader_variable color, const no::rectangle& rectangle) const;
	bool is_visible() const;

private:

	game_state* game = nullptr;
	no::transform2 transform;
	int target_id = -1;
	int texture = -1;
	float wait = 0.0f;
	float fade_in = 0.0f;
	float stay = 0.0f;
	float fade_out = 0.0f;
	float alpha = 0.0f;
	int background = -1;

};

class context_menu {
public:

	context_menu(const no::ortho_camera& camera, no::vector2f position, const no::font& font, no::mouse& mouse);
	context_menu(const context_menu&) = delete;
	context_menu(context_menu&&) = delete;

	~context_menu();

	context_menu& operator=(const context_menu&) = delete;
	context_menu& operator=(context_menu&&) = delete;

	void add_option(const std::string& text, const std::function<void()>& action);
	void trigger(int index);

	bool is_mouse_over(int index) const;
	void draw(int ui_texture) const;
	int count() const;

	float calculate_max_offset_x() const;
	no::transform2 menu_transform() const;
	no::transform2 option_transform(int index) const;

private:

	void draw_border(int uv, no::vector2f offset) const;
	void draw_top_border() const;
	void draw_background() const;
	void draw_side_borders() const;
	void draw_bottom_border() const;
	void draw_options(int ui_texture) const;
	void draw_option(int option, int ui_texture) const;
	void draw_highlighted_option(int option, int ui_texture) const;

	static const int top_begin = 0;
	static const int top_tile_1 = 1;
	static const int top_tile_2 = 2;
	static const int top_tile_3 = 3;
	static const int top_end = 4;
	static const int highlight_begin = 5;
	static const int highlight_tile = 6;
	static const int highlight_end = 7;
	static const int item_tile = 8;
	static const int left_1 = 9;
	static const int left_2 = 10;
	static const int right_1 = 11;
	static const int right_2 = 12;
	static const int bottom_begin = 13;
	static const int bottom_tile_1 = 14;
	static const int bottom_tile_2 = 15;
	static const int bottom_tile_3 = 16;
	static const int bottom_end = 17;
	static const int total_tiles = 18;

	struct menu_option {
		std::string text;
		std::function<void()> action;
		int texture = -1;
	};

	no::vector2f position;
	float max_width = 0.0f;
	std::vector<menu_option> options;
	const no::font& font;
	const no::ortho_camera& camera;
	no::mouse& mouse;
	std::vector<no::rectangle> rectangles;
	no::rectangle full;

	mutable no::shader_variable color;

};

class inventory_view {
public:

	inventory_view(const no::ortho_camera& camera, game_state& game, world_state& world);
	inventory_view(const inventory_view&) = delete;
	inventory_view(inventory_view&&) = delete;

	~inventory_view();

	inventory_view& operator=(const inventory_view&) = delete;
	inventory_view& operator=(inventory_view&&) = delete;

	void listen(int object_id);
	void ignore();

	no::transform2 body_transform() const;
	no::transform2 slot_transform(int index) const;
	void draw() const;
	void add_context_options(context_menu& context);

	no::vector2i hovered_slot() const;

private:

	void on_item_added(const item_container::add_event& event);
	void on_item_removed(const item_container::remove_event& event);

	const no::ortho_camera& camera;
	world_state& world;
	game_state& game;

	int object_id = -1;

	struct item_slot {
		no::rectangle rectangle;
		item_instance item;
	};

	no::rectangle background;
	std::unordered_map<int, item_slot> slots;
	int add_item_event = -1;
	int remove_item_event = -1;

};

class equipment_view {
public:

	equipment_view(const no::ortho_camera& camera, game_state& game, world_state& world);
	equipment_view(const equipment_view&) = delete;
	equipment_view(equipment_view&&) = delete;

	~equipment_view();

	equipment_view& operator=(const equipment_view&) = delete;
	equipment_view& operator=(equipment_view&&) = delete;

	void listen(int object_id);
	void ignore();

	no::transform2 body_transform() const;
	no::transform2 slot_transform(equipment_slot slot) const;
	void draw() const;
	void add_context_options(context_menu& context);

	equipment_slot hovered_slot() const;

private:

	void on_item_added(const item_container::add_event& event);
	void on_item_removed(const item_container::remove_event& event);

	const no::ortho_camera& camera;
	world_state& world;
	game_state& game;

	int object_id = -1;

	struct item_slot {
		no::rectangle rectangle;
		item_instance item;
	};

	no::rectangle background;
	std::unordered_map<equipment_slot, item_slot> slots;
	int add_item_event = -1;
	int remove_item_event = -1;

};

class hud_view {
public:

	std::vector<hit_splat> hit_splats;

	hud_view();

	void update(const no::ortho_camera& camera);
	void draw(no::shader_variable color, int ui_texture, character_object* player) const;
	void set_fps(long long fps);
	void set_debug(const std::string& debug);

private:

	no::rectangle rectangle;
	no::font font;
	int fps_texture = -1;
	int debug_texture = -1;

	long long fps = 0;

	no::rectangle hud_left;
	no::rectangle hud_tile_1;
	no::rectangle hud_tile_2;
	no::rectangle hud_right;
	no::rectangle health_background;
	no::rectangle health_foreground;

};

class user_interface_view {
public:

	hud_view hud;
	world_minimap minimap;
	no::ortho_camera camera;

	user_interface_view(game_state& game, world_state& world);
	user_interface_view(const user_interface_view&) = delete;
	user_interface_view(user_interface_view&&) = delete;

	~user_interface_view();

	user_interface_view& operator=(const user_interface_view&) = delete;
	user_interface_view& operator=(user_interface_view&&) = delete;

	bool is_mouse_over() const;
	bool is_mouse_over_context() const;
	bool is_mouse_over_inventory() const;
	bool is_tab_hovered(int index) const;
	bool is_mouse_over_any() const;
	bool is_context_open() const;

	void listen(int object_id);
	void ignore();

	void update();
	void draw() const;

private:

	inventory_view inventory;
	equipment_view equipment;
	context_menu* context = nullptr;
	
	void draw_tabs() const;
	void draw_tab(int index, const no::rectangle& tab) const;

	no::transform2 tab_transform(int index) const;

	void create_context();

	world_state& world;
	game_state& game;

	int ui_texture = -1;

	no::rectangle background;

	struct {
		no::rectangle background;
		no::rectangle inventory;
		no::rectangle equipment;
		no::rectangle quests;
		int active = 0;
	} tabs;

	int shader = -1;
	int object_id = -1;
	int equipment_event = -1;

	no::shader_variable color;

	int press_event_id = -1;
	int cursor_icon_id = -1;

	no::font font;

};
