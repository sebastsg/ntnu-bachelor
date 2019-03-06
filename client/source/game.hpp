#pragma once

#include "world.hpp"
#include "render.hpp"
#include "ui.hpp"
#include "dialogue_ui.hpp"
#include "chat_ui.hpp"
#include "quest.hpp"

#include "client.hpp"
#include "camera.hpp"
#include "draw.hpp"
#include "assets.hpp"
#include "font.hpp"

class hud_view {
public:

	std::vector<hit_splat> hit_splats;
	no::ortho_camera camera;

	hud_view();

	void update();
	void draw() const;
	void set_fps(long long fps);
	void set_debug(const std::string& debug);

private:

	no::rectangle rectangle;
	no::font font;
	int shader = -1;
	int fps_texture = -1;
	int debug_texture = -1;
	no::shader_variable color;
	
	long long fps = 0;

};

class game_world : public world_state {
public:
	
	int my_player_id = -1;

	game_world();

	character_object* my_player();

private:

	no::io_stream stream;

};

class game_state : public client_state {
public:
	
	game_world world;
	game_variable_map variables;
	quest_instance_list quests;

	game_state();
	~game_state() override;

	void update() override;
	void draw() override;

	const no::font& font() const;
	no::vector2i hovered_tile() const;
	const no::perspective_camera& world_camera() const;

	void start_dialogue(int target_id);
	void close_dialogue();

	void start_combat(int target_id);

	void equip_from_inventory(no::vector2i slot);

	chat_view chat;

private:

	no::vector3i hovered_pixel;

	int mouse_press_id = -1;
	int mouse_scroll_id = -1;
	int keyboard_press_id = -1;
	int receive_packet_id = -1;

	world_view renderer;
	no::font ui_font;
	hud_view hud;
	user_interface_view ui;
	dialogue_view* dialogue = nullptr;

	no::perspective_camera::drag_controller dragger;
	no::perspective_camera::rotate_controller rotater;
	no::perspective_camera::follow_controller follower;
	std::vector<no::vector2i> path_found;

};
