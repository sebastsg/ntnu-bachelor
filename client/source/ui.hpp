#pragma once

#include "world.hpp"
#include "draw.hpp"
#include "font.hpp"

class game_state;

class context_menu {
public:

	context_menu(const no::ortho_camera& camera, no::vector2f position, const no::font& font, no::mouse& mouse);
	context_menu(const context_menu&) = delete;
	context_menu(context_menu&&) = delete;

	~context_menu();

	context_menu& operator=(const context_menu&) = delete;
	context_menu& operator=(context_menu&&) = delete;

	void trigger(int index);
	bool is_mouse_over(int index) const;
	int count() const;
	void add_option(const std::string& text, const std::function<void()>& action);
	void draw() const;

	no::transform menu_transform() const;

private:

	no::transform option_transform(int index) const;

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
	no::rectangle full;
	no::rectangle top;
	no::rectangle row;
	no::rectangle bottom;

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

	void listen(player_object* player);
	void ignore();

	no::transform body_transform() const;
	no::transform slot_transform(int index) const;
	void draw() const;

	no::vector2i hovered_slot() const;

private:

	const no::ortho_camera& camera;
	world_state& world;
	game_state& game;

	player_object* player = nullptr;

	struct inventory_slot {
		no::rectangle rectangle;
		item_instance item;
	};

	no::rectangle background;
	std::unordered_map<int, inventory_slot> slots;
	int add_item_event = -1;
	int remove_item_event = -1;

};

class user_interface_view {
public:
	

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

	void listen(player_object* player);
	void ignore();

	void update();
	void draw() const;

private:

	no::rectangle hud_background;
	inventory_view inventory;
	context_menu* context = nullptr;
	
	void draw_hud() const;

	void draw_tabs() const;
	void draw_tab(int index, const no::rectangle& tab) const;

	no::transform tab_transform(int index) const;

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
	player_object* player = nullptr;
	int equipment_event = -1;

	no::shader_variable color;

	int press_event_id = -1;

	no::font font;

};
