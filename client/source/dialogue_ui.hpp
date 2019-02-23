#pragma once

#include "camera.hpp"
#include "font.hpp"
#include "draw.hpp"
#include "dialogue.hpp"

class game_state;

class text_view {
public:

	no::transform transform;

	text_view();
	text_view(const text_view&) = delete;
	text_view(text_view&&);

	~text_view();

	text_view& operator=(const text_view&) = delete;
	text_view& operator=(text_view&&);

	std::string text() const;
	void render(const no::font& font, const std::string& text);
	void draw(const no::rectangle& rectangle) const;

private:

	std::string rendered_text;
	int texture = -1;

};

class dialogue_view {
public:

	dialogue_view(game_state& game, const no::ortho_camera& camera, int id);
	~dialogue_view();

	void update();
	void draw() const;

	bool is_open() const;

private:

	int process_non_ui_node(int id, node_type type);
	int process_nodes_get_choice(int id, node_type type);
	void prepare_message();
	void select_choice(int node_id);
	void process_entry_point();
	void start_dialogue();

	game_state& game;
	const no::ortho_camera& camera;
	int key_listener = -1;

	bool open = true;
	dialogue_tree dialogue;
	int current_node_id = 0;
	std::vector<dialogue_choice_button> choices;
	int current_choice = 0;
	no::transform transform;

	no::font font;

	text_view message_view;
	std::vector<text_view> choice_views;
	no::rectangle rectangle;

};
