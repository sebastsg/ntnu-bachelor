#pragma once

#include "client.hpp"
#include "world.hpp"
#include "render.hpp"
#include "quest.hpp"
#include "gamevar.hpp"

struct player_data {
	character_object& character;
	game_object& object;
};

class game_world : public world_state {
public:
	
	int my_player_id = -1;

	game_world();

	player_data my_player();

private:

	no::io_stream stream;

};

class game_state : public client_state {
public:
	
	game_world world;
	game_variable_map variables;
	quest_instance_list quests;
	no::ortho_camera ui_camera_2x;
	no::ortho_camera ui_camera_1x;

	game_state();
	~game_state() override;

	void update() override;
	void draw() override;

	no::vector2i hovered_tile() const;
	const no::perspective_camera& world_camera() const;

	void start_dialogue(int target_id);

	void start_combat(int target_id);
	void follow_character(int target_id);

	void equip_from_inventory(no::vector2i slot);
	void unequip_to_inventory(equipment_slot slot);
	void consume_from_inventory(no::vector2i slot);

	void send_trade_request(int target_id);
	void send_add_trade_item(const item_instance& item);
	void send_remove_trade_item(no::vector2i slot);
	void send_finish_trading(bool accept);

	void send_start_fishing(no::vector2i tile);

private:

	no::vector3i hovered_pixel;

	int cursor_icon_id = -1;
	int mouse_press_id = -1;
	int mouse_scroll_id = -1;
	int keyboard_press_id = -1;
	int receive_packet_id = -1;

	world_view renderer;

	no::perspective_camera::drag_controller dragger;
	no::perspective_camera::rotate_controller rotater;
	no::perspective_camera::follow_controller follower;
	std::vector<no::vector2i> path_found;

	int sent_trade_request_to_player_id = -1;

};
