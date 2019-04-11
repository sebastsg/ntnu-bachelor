#pragma once

#include "world.hpp"
#include "draw.hpp"
#include "character.hpp"
#include "ui.hpp"
#include "minimap.hpp"
#include "ui_stats.hpp"

class game_state;

const no::vector2f ui_size = 1024.0f;
const no::vector2f item_size = 32.0f;
const no::vector2f item_grid = item_size + 2.0f;
const no::vector4f background_uv = { 391.0f, 48.0f, 184.0f, 352.0f };
const no::vector2f tab_background_uv = { 128.0f, 24.0f };
const no::vector2f tab_inventory_uv = { 160.0f, 24.0f };
const no::vector2f tab_equipment_uv = { 224.0f, 24.0f };
const no::vector2f tab_quests_uv = { 192.0f, 24.0f };
const no::vector2f tab_stats_uv = { 256.0f, 24.0f };
const no::vector2f tab_size = 24.0f;

const no::vector4f hud_left_uv = { 8.0f, 528.0f, 40.0f, 68.0f };
const no::vector4f hud_tile_1_uv = { 56.0f, 528.0f, 16.0f, 68.0f };
const no::vector4f hud_tile_2_uv = { 80.0f, 528.0f, 16.0f, 68.0f };
const no::vector4f hud_right_uv = { 104.0f, 528.0f, 16.0f, 68.0f };
const no::vector2f hud_health_background = { 104.0f, 152.0f };
const no::vector2f hud_health_foreground = { 112.0f, 152.0f };
const no::vector2f hud_health_size = { 8.0f, 12.0f };

const no::vector4f inventory_uv = { 200.0f, 128.0f, 138.0f, 205.0f };
const no::vector2f inventory_offset = { 23.0f, 130.0f };

const no::vector4f equipment_uv = { 199.0f, 336.0f, 138.0f, 205.0f };

void set_ui_uv(no::rectangle& rectangle, no::vector2f uv, no::vector2f uv_size);
void set_ui_uv(no::rectangle& rectangle, no::vector4f uv);
void set_item_uv(no::rectangle& rectangle, no::vector2f uv);

struct item_slot {
	no::rectangle rectangle;
	item_instance item;
};

class inventory_view {
public:

	inventory_view(game_state& game);
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
	void add_context_options();

	no::vector2i hovered_slot() const;

private:

	void on_change(no::vector2i slot);

	game_state& game;

	int object_id = -1;

	no::rectangle background;
	std::unordered_map<int, item_slot> slots;
	int change_event = -1;

};

class equipment_view {
public:

	equipment_view(game_state& game);
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
	void add_context_options();

	equipment_slot hovered_slot() const;

private:

	void on_change(equipment_slot slot);

	game_state& game;

	int object_id = -1;

	no::rectangle background;
	std::unordered_map<equipment_slot, item_slot> slots;
	int change_event = -1;

};

class user_interface_view {
public:

	world_minimap minimap;

	user_interface_view(game_state& game);
	user_interface_view(const user_interface_view&) = delete;
	user_interface_view(user_interface_view&&) = delete;

	~user_interface_view();

	user_interface_view& operator=(const user_interface_view&) = delete;
	user_interface_view& operator=(user_interface_view&&) = delete;

	bool is_mouse_over() const;
	bool is_mouse_over_inventory() const;
	bool is_tab_hovered(int index) const;
	bool is_mouse_over_any() const;

	void listen(int object_id);
	void ignore();

	void update();
	void draw();

private:

	inventory_view inventory;
	equipment_view equipment;
	stats_view stats;
	
	void draw_tabs() const;
	void draw_tab(int index, const no::rectangle& tab) const;

	no::transform2 tab_transform(int index) const;

	void create_context();

	game_state& game;

	no::rectangle background;

	struct {
		no::rectangle background;
		no::rectangle inventory;
		no::rectangle equipment;
		no::rectangle quests;
		no::rectangle stats;
		int active = 0;
	} tabs;

	int object_id = -1;
	int equipment_event = -1;

	int press_event_id = -1;
	int cursor_icon_id = -1;

};
