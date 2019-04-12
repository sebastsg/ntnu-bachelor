#pragma once

#include "event.hpp"

#include <string>

class game_state;

void show_chat(game_state& game);
void hide_chat();
void update_chat();
void draw_chat();
void add_chat_message(const std::string& author, const std::string& message);
void enable_chat();
void disable_chat();
no::message_event<std::string>& chat_message_event();
