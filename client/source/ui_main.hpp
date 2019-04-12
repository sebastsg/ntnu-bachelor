#pragma once

#include "world.hpp"
#include "draw.hpp"
#include "character.hpp"
#include "ui.hpp"

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

class user_interface_view {
public:

	user_interface_view(game_state& game);
	user_interface_view(const user_interface_view&) = delete;
	user_interface_view(user_interface_view&&) = delete;

	~user_interface_view();

	user_interface_view& operator=(const user_interface_view&) = delete;
	user_interface_view& operator=(user_interface_view&&) = delete;

	bool is_mouse_over() const;
	bool is_tab_hovered(int index) const;
	bool is_mouse_over_any() const;

	void listen(int object_id);
	void ignore();

	void update();
	void draw();

private:
	
	void draw_tabs() const;
	void draw_tab(int index, const no::rectangle& tab) const;

	no::transform2 tab_transform(int index) const;

	void create_context();

	game_state& game;

	struct {
		no::rectangle background;
		no::rectangle inventory;
		no::rectangle equipment;
		no::rectangle quests;
		no::rectangle stats;
		int active = 0;
	} tabs;

	int object_id = -1;

	int press_event_id = -1;
	int cursor_icon_id = -1;

};
