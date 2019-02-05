#pragma once

#include "player.hpp"
#include "world.hpp"
#include "render.hpp"
#include "ui.hpp"

#include "loop.hpp"
#include "camera.hpp"
#include "draw.hpp"
#include "assets.hpp"
#include "font.hpp"
#include "network.hpp"

class hud_view {
public:

	no::ortho_camera camera;

	hud_view();

	void draw() const;
	void set_fps(long long fps);
	void set_debug(const std::string& debug);

private:

	no::rectangle rectangle;
	no::font font;
	int shader = -1;
	int fps_texture = -1;
	int debug_texture = -1;
	
	long long fps = 0;

};

class game_world : public world_state {
public:
	
	int my_player_id = -1;

	game_world();

	player_object* my_player();

private:

	no::io_stream stream;

};

class game_state : public no::window_state {
public:

	game_state();
	~game_state() override;

	void update() override;
	void draw() override;

private:

	no::vector3i hovered_pixel;

	no::io_socket server;

	int mouse_press_id = -1;
	int mouse_scroll_id = -1;
	int keyboard_press_id = -1;

	game_world world;
	world_view renderer;
	hud_view hud;
	user_interface_view ui;

	no::perspective_camera::drag_controller dragger;
	no::perspective_camera::rotate_controller rotater;
	no::perspective_camera::follow_controller follower;

};
