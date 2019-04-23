#pragma once

#include "draw.hpp"

class game_state;

const no::vector2f inventory_offset = { 23.0f, 198.0f };
const no::vector2f ui_size = 1024.0f;
const no::vector2f item_size = 32.0f;
const no::vector2f item_grid = item_size + 2.0f;
const no::vector4f background_uv = { 672.0f, 572.0f, 190.0f, 284.0f };
const no::vector2f tab_background_uv = { 128.0f, 26.0f };
const no::vector2f tab_inventory_uv = { 160.0f, 24.0f };
const no::vector2f tab_equipment_uv = { 224.0f, 24.0f };
const no::vector2f tab_quests_uv = { 192.0f, 24.0f };
const no::vector2f tab_stats_uv = { 256.0f, 24.0f };
const no::vector2f tab_size = 24.0f;

void set_ui_uv(no::rectangle& rectangle, no::vector2f uv, no::vector2f uv_size);
void set_ui_uv(no::rectangle& rectangle, no::vector4f uv);
void set_item_uv(no::rectangle& rectangle, no::vector2f uv);
no::transform2 tab_body_transform();
void show_tabs(game_state& game);
void hide_tabs();
void update_tabs();
void draw_tabs();
void add_tabs_context_menu_options();
void enable_tabs();
void disable_tabs();
bool is_mouse_over_tabs();
bool is_mouse_over_tab_body();
