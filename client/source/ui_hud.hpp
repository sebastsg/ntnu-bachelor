#pragma once

#include "draw.hpp"
#include "font.hpp"
#include "character.hpp"

class game_state;

void show_hud(game_state& game);
void hide_hud();
void update_hud();
void draw_hud(int ui_texture);

void set_hud_fps(long long fps);
void set_hud_debug(const std::string& debug);
