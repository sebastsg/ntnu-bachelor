#pragma once

#include "menu.hpp"
#include "object.hpp"
#include "draw.hpp"

class object_editor : public menu_bar_state {
public:

	object_editor();
	~object_editor() override;

	void update() override;
	void draw() override;

private:

	void object_type_combo(game_object_type& type, const std::string& ui_id);

	void ui_create_object();
	void ui_select_object();

	int current_object = -1;

	struct {
		game_object_type type = game_object_type::decoration;
		char name[100] = {};
		char model[100] = {};
	} new_object;

};
