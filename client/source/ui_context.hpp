#pragma once

#include <functional>

class game_state;

void open_context_menu(game_state& game);
void close_context_menu();
bool is_context_menu_open();
void add_context_menu_option(const std::string& text, const std::function<void()>& action);
void trigger_context_menu_option(int index);
bool is_mouse_over_context_menu();
bool is_mouse_over_context_menu_option(int index);
void draw_context_menu();
int context_menu_option_count();
