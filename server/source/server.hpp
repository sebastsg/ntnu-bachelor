#pragma once

#include "updater.hpp"

#include "loop.hpp"
#include "network.hpp"
#include "world.hpp"
#include "character.hpp"
#include "packets.hpp"

class client_state {
public:

	int player_instance_id = -1;

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

};

class server_world : public world_state {
public:

	server_world(const std::string& name);

};

class server_state : public no::window_state {
public:

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

	void on_move_to_tile(int client_index, const packet::game::move_to_tile& packet);
	void on_connect_to_world(int client_index, const packet::lobby::connect_to_world& packet);
	void on_version_check(int client_index, const packet::updates::version_check& packet);

	no::connection_establisher establisher;
	no::socket_container sockets;
	client_state clients[max_clients];

	std::vector<server_world> worlds;

	std::vector<client_updater> updaters;

};
