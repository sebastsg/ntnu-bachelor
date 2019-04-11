#pragma once

#include "ui_main.hpp"

class chat_view {
public:

	struct message_event {
		std::string message;
	};

	struct {
		no::message_event<message_event> message;
	} events;

	chat_view(const game_state& game);
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

	const game_state& game;
	int key_press = -1;
	int key_input = -1;

	no::text_view input;
	std::vector<no::text_view> history;

	int visible_history_length = 10;

};
