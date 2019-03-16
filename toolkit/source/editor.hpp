#pragma once

#include "menu.hpp"
#include "camera.hpp"
#include "draw.hpp"
#include "assets.hpp"
#include "font.hpp"
#include "world.hpp"
#include "render.hpp"

class world_editor_state;

class editor_world : public world_state {
public:

	editor_world();

	void update();

private:


};

class world_editor_tool {
public:

	world_editor_tool(world_editor_state& editor);
	virtual ~world_editor_tool() = default;

	virtual void enable() {}
	virtual void disable() {}
	virtual void update() = 0;
	virtual void update_imgui() = 0;
	virtual void draw() = 0;

protected:

	world_editor_state& editor;
	
};

class elevate_tool : public world_editor_tool {
public:

	elevate_tool(world_editor_state& editor);

	void update() override;
	void update_imgui() override;
	void draw() override;

private:

	float elevation_rate = 0.05f;
	bool limit_elevation = false;
	float elevation_limit = 1.0f;

};

class tiling_tool : public world_editor_tool {
public:

	tiling_tool(world_editor_state& editor);

	void update() override;
	void update_imgui() override;
	void draw() override;

private:

	int current_type = 0;

};

class tile_flag_tool : public world_editor_tool {
public:

	tile_flag_tool(world_editor_state& editor);

	void update() override;
	void update_imgui() override;
	void draw() override;

	void refresh();

private:

	void apply();
	void erase(no::vector2i tile);

	int flag = world_tile::solid_flag;
	bool value = false;
	std::vector<no::vector2i> tiles_with_flag;

};

class object_tool : public world_editor_tool {
public:

	object_tool(world_editor_state& editor);
	~object_tool() override;

	void enable() override;
	void disable() override;
	void update() override;
	void update_imgui() override;
	void draw() override;

private:

	int object_definition_id = 0;
	game_object object;

	int mouse_press_id = -1;
	no::vector2i context_menu_tile;
	int selected_object_instance_id = -1;
	int ui_texture = -1;

};

class world_editor_state : public menu_bar_state {
public:

	editor_world world;
	world_view renderer;
	int brush_size = 1;
	no::vector2i selected_tile = -1;
	std::vector<no::vector2i> brush_tiles;
	no::vector3i hovered_pixel;
	no::vector2i hovered_tile;

	world_editor_state();
	~world_editor_state() override;

	void update() override;
	void draw() override;

	bool is_mouse_over_ui() const;
	no::vector3f world_position_for_tile(no::vector2i tile) const;

private:

	static const int elevate_tool_id = 0;
	static const int tiling_tool_id = 1;
	static const int object_tool_id = 2;
	static const int tile_flag_tool_id = 3;

	void update_editor();
	void update_imgui();

	world_editor_tool& tool();

	int current_tool_id = 0;
	elevate_tool elevate;
	tiling_tool tiling;
	object_tool object;
	tile_flag_tool tile_flag;

	bool show_wireframe = false;
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
