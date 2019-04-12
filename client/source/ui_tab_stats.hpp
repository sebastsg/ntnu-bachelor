#pragma once

class game_state;

void show_stats_tab(game_state& game);
void hide_stats_tab();
void update_stats_tab();
void draw_stats_tab();
void add_stats_context_menu_options();
bool is_mouse_over_stats();
