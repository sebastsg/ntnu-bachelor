#pragma once

#include "main_ui.hpp"

class chat_view {
public:

	struct message_event {
		std::string message;
	};

	struct {
		no::message_event<message_event> message;
	} events;

	chat_view(game_state& game, const no::ortho_camera& camera);
	chat_view(const chat_view&) = delete;
	chat_view(chat_view&&) = delete;

	~chat_view();

	chat_view& operator=(const chat_view&) = delete;
	chat_view& operator=(chat_view&&) = delete;

	void update();
	void draw() const;

	void add(const std::string& author, const std::string& message);
	void enable();
	void disable();

private:

	game_state& game;
	const no::ortho_camera& camera;
	int key_press = -1;
	int key_input = -1;

	no::text_view input;
	std::vector<no::text_view> history;
	no::rectangle rectangle;

	int visible_history_length = 10;

};
