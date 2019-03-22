#pragma once

#include "menu.hpp"
#include "object.hpp"
#include "skeletal.hpp"
#include "camera.hpp"

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
	bool is_mouse_over_ui() const;

	int current_object = -1;

	struct {
		game_object_type type = game_object_type::decoration;
		char name[100] = {};
		char model[100] = {};
	} new_object;

	no::model model;
	int model_texture = -1;
	int blank_texture = -1;
	no::model box;
	no::skeletal_animator animator;

	int mouse_scroll_id = -1;
	int animate_shader = -1;
	int static_shader = -1;

	no::perspective_camera camera;
	no::perspective_camera::drag_controller dragger;

	float rotation = 0.0f;

};
