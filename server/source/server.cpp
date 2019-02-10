#include "server.hpp"
#include "debug.hpp"
#include "io.hpp"
#include "platform.hpp"
#include "assets.hpp"
#include "packets.hpp"

server_world::server_world(const std::string& name) {
	load(no::asset_path("worlds/" + name + ".ew"));
}

server_state::server_state() {
	worlds.emplace_back("main");
	
	establisher.container = &sockets;
	establisher.listen("10.0.0.130", 7524); // todo: config file
	establisher.events.established.listen([this](const no::connection_establisher::established_message& event) {
		connect((int)event.index);
	});
}

server_state::~server_state() {
	
}

void server_state::update() {
	establisher.synchronise();
	sockets.synchronise();
}

void server_state::draw() {
	
}

void server_state::connect(int index) {
	if (index >= max_clients) {
		// todo: send some message to client to let it know properly
		WARNING("Rejecting client. Connection limit reached");
		return;
	}
	if (clients[index].is_connected()) {
		// todo: check if this can happen
		WARNING("Rejecting client. This slot is already in use.");
		return;
	}
	INFO("Connecting client " << index);
	clients[index] = { true };

	// todo: load player from database
	auto player = (character_object*)worlds.front().objects.add(1);
	player->change_id(worlds.front().objects.next_dynamic_id());
	player->inventory.resize({ 4, 6 });
	player->equipment.resize({ 3, 3 });
	clients[index].player_instance_id = player->id();
	item_instance item;
	item.definition_id = 0;
	item.stack = 1;
	player->inventory.add_from(item);
	item.definition_id = 1;
	item.stack = 1;
	player->inventory.add_from(item);
	item.definition_id = 2;
	item.stack = 1;
	player->inventory.add_from(item);
	item.definition_id = 3;
	item.stack = 1;
	player->inventory.add_from(item);
	player->transform.position.x = 15.0f;
	player->transform.position.z = 15.0f;
	
	// send packets
	player_joined_packet joined;
	joined.is_me = 1;
	joined.player = *player;
	no::io_stream session_stream;
	no::packetizer::start(session_stream);
	joined.write(session_stream);
	no::packetizer::end(session_stream);
	sockets[index].send_async(session_stream);

	for (int j = 0; j < max_clients; j++) {
		if (clients[j].is_connected() && index != j) {
			joined.is_me = 0;
			joined.player = *(character_object*)worlds.front().objects.find(clients[j].player_instance_id);
			session_stream = {};
			no::packetizer::start(session_stream);
			joined.write(session_stream);
			no::packetizer::end(session_stream);
			sockets[index].send_async(session_stream);
		}
	}

	session_stream = {};
	joined.player = *(character_object*)worlds.front().objects.find(clients[index].player_instance_id);
	joined.is_me = 0;
	no::packetizer::start(session_stream);
	joined.write(session_stream);
	no::packetizer::end(session_stream);
	sockets.broadcast<false>(session_stream, index);

	sockets[index].events.receive_packet.listen([this, index](const no::io_socket::receive_packet_message& event) {
		no::io_stream stream = { event.packet.data(), event.packet.size(), no::io_stream::construct_by::shallow_copy };
		int16_t type = stream.read<int16_t>();
		switch (type) {
		case move_to_tile_packet::type:
		{
			move_to_tile_packet packet;
			packet.read(stream);

			packet.player_instance_id = clients[index].player_instance_id;
			// todo: update server world

			no::io_stream packet_stream;
			no::packetizer::start(packet_stream);
			packet.write(packet_stream);
			no::packetizer::end(packet_stream);
			sockets.broadcast<true>(packet_stream);
			break;
		}
		default:
			break;
		}
	});

	sockets[index].events.disconnect.listen([this, index](const no::io_socket::disconnect_message& event) {
		INFO("Client " << index << " has disconnected with status " << event.status);
		clients[index] = { false };

		no::io_stream disconnect_stream;
		player_disconnected_packet disconnection;
		disconnection.player_instance_id = clients[index].player_instance_id;
		no::packetizer::start(disconnect_stream);
		disconnection.write(disconnect_stream);
		no::packetizer::end(disconnect_stream);
		sockets.broadcast<true>(disconnect_stream);
	});
}

void server_state::disconnect(int index) {
	if (index >= max_clients) {
		return;
	}
	clients[index] = { false };
	INFO("Disconnecting client " << index);
}

void configure() {
#if _DEBUG
	no::set_asset_directory("../../../assets");
#else
	no::set_asset_directory("../../../assets");
	//no::set_asset_directory("assets");
#endif
}

void start() {
	no::create_state<server_state>("Server", 800, 600, 4, false);
}
