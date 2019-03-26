#pragma once

#include "main_ui.hpp"
#include "script.hpp"

class dialogue_view {
public:

	struct choose_event {
		int choice = -1;
	};

	struct {
		no::message_event<choose_event> choose;
	} events;

	dialogue_view(game_state& game, const no::ortho_camera& camera, int id);
	~dialogue_view();

	void update();
	void draw() const;

	bool is_open() const;

private:

	game_state& game;
	const no::ortho_camera& camera;
	int key_listener = -1;

	bool open = true;
	script_tree dialogue;
	int current_choice = 0;
	std::vector<node_choice_info> current_choices;
	no::transform2 transform;

	no::font font;

	no::text_view message_view;
	std::vector<no::text_view> choice_views;
	no::rectangle rectangle;

};
