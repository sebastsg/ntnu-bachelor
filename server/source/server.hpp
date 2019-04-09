#pragma once

#include "updater.hpp"
#include "persistency.hpp"
#include "loop.hpp"
#include "network.hpp"
#include "world.hpp"
#include "character.hpp"
#include "packets.hpp"
#include "script.hpp"
#include "quest.hpp"
#include "combat.hpp"

const int inventory_container_type = 0;
const int equipment_container_type = 1;
const int warehouse_container_type = 2;

class client_state {
public:
	
	bool connected = false;
	no::timer last_saved;

	int sent_trade_request_to_player_id = -1;

	struct {
		std::string email;
	} account;

	struct {
		int id = -1;
		std::string display_name;
		game_variable_map variables;
		quest_instance_list quests;
	} player;

	struct {
		int player_instance_id = -1;
	} object;

	script_tree* dialogue = nullptr;

	client_state() = default;
	client_state(bool connected) : connected(connected) {}
	client_state(const client_state&) = delete;
	client_state(client_state&&) = default;

	client_state& operator=(const client_state&) = delete;
	client_state& operator=(client_state&&) = default;

	bool is_connected() const {
		return connected;
	}

};

class server_world : public world_state {
public:

	struct kill_event {
		int attacker_id = -1;
		int target_id = -1;
	};

	struct {
		no::event_message_queue<kill_event> kill;
	} events;

	combat_system combat;

	struct fishing_state {
		no::timer fished_for;
		no::timer last_progress;
		int client_index = -1;
		int player_instance_id = -1;
		no::vector2i bait_tile;
		bool finished = false;
	};

	std::vector<fishing_state> fishers;

	server_world(const std::string& name);

	void update() override;

};

struct trade_state {

	inventory_container first;
	inventory_container second;

	int first_client_index = -1;
	int second_client_index = -1;

	bool first_accepts = false;
	bool second_accepts = false;

};

class server_state : public no::window_state {
public:

	static const int max_clients = 100;

	database_connection database;
	game_persister persister;

	server_state();
	~server_state() override;

	void update() override;
	void draw() override;

private:

	int client_with_player(int player_id);

	character_object* load_player(int client_index);
	void save_player(int client_index);

	void connect(int index);

	void on_receive_packet(int client_index, int16_t type, no::io_stream& stream);
	void on_disconnect(int client_index);

	void on_move_to_tile(int client_index, const to_server::game::move_to_tile& packet);
	void on_start_dialogue(int client_index, const to_server::game::start_dialogue& packet);
	void on_continue_dialogue(int client_index, const to_server::game::continue_dialogue& packet);
	void on_chat_message(int client_index, const to_server::game::chat_message& packet);
	void on_start_combat(int client_index, const to_server::game::start_combat& packet);
	void on_equip_from_inventory(int client_index, const to_server::game::equip_from_inventory& packet);
	void on_unequip_to_inventory(int client_index, const to_server::game::unequip_to_inventory& packet);
	void on_follow_character(int client_index, const to_server::game::follow_character& packet);
	void on_trade_request(int client_index, const to_server::game::trade_request& packet);
	void on_add_trade_item(int client_index, const to_server::game::add_trade_item& packet);
	void on_remove_trade_item(int client_index, const to_server::game::remove_trade_item& packet);
	void on_trade_decision(int client_index, const to_server::game::trade_decision& packet);
	void on_started_fishing(int client_index, const to_server::game::started_fishing& packet);
	void on_consume_from_inventory(int client_index, const to_server::game::consume_from_inventory& packet);

	void on_login_attempt(int client_index, const to_server::lobby::login_attempt& packet);
	void on_connect_to_world(int client_index, const to_server::lobby::connect_to_world& packet);
	void on_update_query(int client_index, const to_server::updates::update_query& packet);

	trade_state* find_trade(int client_index);
	void remove_trade(int client_index);

	int listener = -1;
	client_state clients[max_clients];

	std::vector<trade_state> trades;

	server_world world;
	int combat_hit_event_id = -1;

	std::vector<client_updater> updaters;

};
