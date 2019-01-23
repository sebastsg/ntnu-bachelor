#pragma once

#include "loop.hpp"
#include "network.hpp"
#include "world.hpp"

class client_state {
public:

	int tile_x = 15;
	int tile_z = 15;

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
	server_world(server_world&&);

private:

	no::io_stream stream;

};

class server_state : public no::window_state {
public:

	static constexpr int max_clients = 100;

	server_state();
	~server_state() override;

	void update() override;
	void draw() override;

private:

	void connect(int index);
	void disconnect(int index);

	no::connection_establisher establisher;
	no::socket_container sockets;
	client_state clients[max_clients];

	std::vector<server_world> worlds;

};
