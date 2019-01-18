#pragma once

#include "loop.hpp"
#include "camera.hpp"
#include "draw.hpp"
#include "assets.hpp"
#include "font.hpp"
#include "world.hpp"

class editor_world : public world_state {
public:

	bool is_dirty = false;

	editor_world();

	void update();

private:


};

class world_editor_state : public no::window_state {
public:

	world_editor_state();
	~world_editor_state() override;

	void update() override;
	void draw() override;

	bool is_mouse_over_ui() const;

private:

	no::vector3i hovered_pixel;
	no::vector2i hovered_tile;

	bool show_wireframe = false;
	int brush_size = 1;

	void update_editor();
	void update_imgui();

	editor_world world;
	world_view renderer;

	no::vector2i selected_tile = -1;
	std::vector<no::vector2i> brush_tiles;
	bool is_selected = false;

	// todo: have an automatic attach/detach system in base class?
	int mouse_press_id = -1;
	int mouse_release_id = -1;
	int mouse_scroll_id = -1;
	int keyboard_press_id = -1;

	no::perspective_camera::drag_controller dragger;
	no::perspective_camera::rotate_controller rotater;
	no::perspective_camera::move_controller mover;

};
