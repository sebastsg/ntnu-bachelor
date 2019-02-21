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
	for (size_t i = 0; i < updaters.size(); i++) {
		updaters[i].update();
		if (updaters[i].is_done()) {
			updaters.erase(updaters.begin() + i);
			i--;
		}
	}
}

void server_state::draw() {
	
}

character_object* server_state::load_player(int client_index) {
	// todo: load player from database
	auto player = (character_object*)worlds.front().objects.add(1);
	player->transform.scale = 0.5f;
	player->change_id(worlds.front().objects.next_dynamic_id());
	player->inventory.resize({ 4, 6 });
	player->equipment.resize({ 3, 3 });
	clients[client_index].player_instance_id = player->id();
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
	return player;
}

void server_state::send_player_joined(int client_index_joined) {

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

	auto player = load_player(index);
	
	player_joined_packet joined;
	joined.is_me = 1;
	joined.player = *player;
	sockets[index].send_async(no::packet_stream(joined));

	for (int j = 0; j < max_clients; j++) {
		if (clients[j].is_connected() && index != j) {
			joined.is_me = 0;
			joined.player = *(character_object*)worlds.front().objects.find(clients[j].player_instance_id);
			sockets[index].send_async(no::packet_stream(joined));
		}
	}

	joined.player = *(character_object*)worlds.front().objects.find(clients[index].player_instance_id);
	joined.is_me = 0;
	sockets.broadcast<false>(no::packet_stream(joined), index);

	sockets[index].events.receive_packet.listen([this, index](const no::io_socket::receive_packet_message& event) {
		no::io_stream stream = { event.packet.data(), event.packet.size(), no::io_stream::construct_by::shallow_copy };
		int16_t type = stream.read<int16_t>();
		on_receive_packet(index, type, stream);
	});

	sockets[index].events.disconnect.listen([this, index](const no::io_socket::disconnect_message& event) {
		INFO("Client " << index << " has disconnected with status " << event.status);
		on_disconnect(index);
	});
}

void server_state::disconnect(int index) {
	if (index >= max_clients) {
		return;
	}
	clients[index] = { false };
	INFO("Disconnecting client " << index);
}

void server_state::on_receive_packet(int client_index, int16_t type, no::io_stream& stream) {
	switch (type) {
	case move_to_tile_packet::type:
	{
		move_to_tile_packet packet;
		packet.read(stream);

		packet.player_instance_id = clients[client_index].player_instance_id;
		// todo: update server world
		sockets.broadcast<true>(no::packet_stream(packet));
		break;
	}
	case version_packet::type:
	{
		version_packet packet;
		packet.read(stream);
		//if (packet.version != newest_client_version) {
			updaters.emplace_back(sockets[client_index]);
		//}
		packet.version = newest_client_version;
		sockets[client_index].send(no::packet_stream(packet));
		break;
	}
	default:
		break;
	}
}

void server_state::on_disconnect(int client_index) {
	clients[client_index] = { false };
	player_disconnected_packet disconnection;
	disconnection.player_instance_id = clients[client_index].player_instance_id;
	sockets.broadcast<true>(no::packet_stream(disconnection));
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
