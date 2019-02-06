#pragma once

#include "world.hpp"
#include "draw.hpp"

class game_state;

class inventory_view {
public:

	inventory_view(game_state& game, world_state& world);
	inventory_view(const inventory_view&) = delete;
	inventory_view(inventory_view&&) = delete;

	~inventory_view();

	inventory_view& operator=(const inventory_view&) = delete;
	inventory_view& operator=(inventory_view&&) = delete;

	void listen(player_object* player);
	void ignore();

	void draw(const no::ortho_camera& camera) const;

private:

	world_state& world;
	game_state& game;

	player_object* player = nullptr;

	struct inventory_slot {
		no::rectangle rectangle;
		item_instance item;
	};

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

	void listen(player_object* player);
	void ignore();

	void draw() const;

private:

	inventory_view inventory;

	no::rectangle hud_background;
	
	void draw_hud() const;

	void draw_tabs() const;
	void draw_tab(int index, const no::rectangle& tab) const;

	no::transform tab_transform(int index) const;
	bool is_tab_hovered(int index) const;

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

};
