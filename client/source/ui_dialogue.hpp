#pragma once

#include "event.hpp"

class game_state;

void open_dialogue(game_state& game, int id);
void close_dialogue();
bool is_dialogue_open();
void update_dialogue();
void draw_dialogue();
no::message_event<int>& dialogue_choice_event();
