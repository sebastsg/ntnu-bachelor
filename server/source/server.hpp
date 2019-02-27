#pragma once

#include "updater.hpp"
#include "persistency.hpp"
#include "loop.hpp"
#include "network.hpp"
#include "world.hpp"
#include "character.hpp"
#include "packets.hpp"
#include "dialogue.hpp"

class client_state {
public:

	friend class server_state;

	client_state() = default;
	client_state(bool connected) : connected(connected) {}
	client_state(const client_state&) = delete;
	client_state(client_state&&) = default;

	client_state& operator=(const client_state&) = delete;
	client_state& operator=(client_state&&) = default;

	bool is_connected() const {
		return connected;
	}

private:

	bool connected = false;
	std::string login_name;
	std::string display_name;
	int player_instance_id = -1;
	dialogue_tree* dialogue = nullptr;
	game_variable_map variables;

};

class server_world : public world_state {
public:

	server_world(const std::string& name);

};

class server_state : public no::window_state {
public:

	database_connection database;

	static constexpr int max_clients = 100;

	server_state();
	~server_state() override;

	void update() override;
	void draw() override;

private:

	character_object* load_player(int client_index);

	void connect(int index);

	void on_receive_packet(int client_index, int16_t type, no::io_stream& stream);
	void on_disconnect(int client_index);

	void send_player_joined(int client_index_joined);

	void on_move_to_tile(int client_index, const to_server::game::move_to_tile& packet);
	void on_start_dialogue(int client_index, const to_server::game::start_dialogue& packet);
	void on_continue_dialogue(int client_index, const to_server::game::continue_dialogue& packet);
	void on_chat_message(int client_index, const to_server::game::chat_message& packet);
	void on_login_attempt(int client_index, const to_server::lobby::login_attempt& packet);
	void on_connect_to_world(int client_index, const to_server::lobby::connect_to_world& packet);
	void on_version_check(int client_index, const to_server::updates::update_query& packet);

	no::connection_establisher establisher;
	no::socket_container sockets;
	client_state clients[max_clients];

	std::vector<server_world> worlds;

	std::vector<client_updater> updaters;

};
