#include "server.hpp"
#include "debug.hpp"
#include "io.hpp"
#include "platform.hpp"
#include "assets.hpp"
#include "packets.hpp"

server_world::server_world(const std::string& name) {
	load(no::asset_path("worlds/" + name + ".ew"));
}

server_state::server_state() : persister(database) {
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
	auto& client = clients[client_index];
	no::vector2i tile = persister.load_player_tile(client.player.id);
	client.object.variables = persister.load_player_variables(client.player.id);

	auto player = (character_object*)worlds.front().objects.add(1);
	player->transform.scale = 0.5f;
	player->change_id(worlds.front().objects.next_dynamic_id());
	player->inventory.resize({ 4, 6 });
	player->equipment.resize({ 3, 3 });
	clients[client_index].object.player_instance_id = player->id();
	item_instance item;
	item.definition_id = 0;
	item.stack = 1;
	player->inventory.add_from(item);
	item.definition_id = 1;
	item.stack = 1;
	player->inventory.add_from(item);
	item.definition_id = 3;
	item.stack = 1;
	player->inventory.add_from(item);
	player->transform.position.x = (float)tile.x;
	player->transform.position.z = (float)tile.y;
	return player;
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

void server_state::on_receive_packet(int client_index, int16_t type, no::io_stream& stream) {
	switch (type) {
	case to_server::game::move_to_tile::type:
		on_move_to_tile(client_index, { stream });
		break;
	case to_server::game::start_dialogue::type:
		on_start_dialogue(client_index, { stream });
		break;
	case to_server::game::continue_dialogue::type:
		on_continue_dialogue(client_index, { stream });
		break;
	case to_server::game::chat_message::type:
		on_chat_message(client_index, { stream });
		break;
	case to_server::lobby::login_attempt::type:
		on_login_attempt(client_index, { stream });
		break;
	case to_server::lobby::connect_to_world::type:
		on_connect_to_world(client_index, { stream });
		break;
	case to_server::updates::update_query::type:
		on_version_check(client_index, { stream });
		break;
	default:
		break;
	}
}

void server_state::on_disconnect(int client_index) {
	clients[client_index] = { false };
	to_client::game::player_disconnected disconnection;
	disconnection.player_instance_id = clients[client_index].object.player_instance_id;
	sockets.broadcast<false>(no::packet_stream(disconnection));
}

void server_state::send_player_joined(int client_index_joined) {

}

void server_state::on_move_to_tile(int client_index, const to_server::game::move_to_tile& packet) {
	auto& client = clients[client_index];
	to_client::game::move_to_tile new_packet;
	new_packet.tile = packet.tile;
	new_packet.player_instance_id = client.object.player_instance_id;
	auto player = worlds.front().objects.find(new_packet.player_instance_id);
	if (player) {
		// todo: pathfinding
		player->transform.position.x = (float)packet.tile.x;
		player->transform.position.z = (float)packet.tile.y;
	}
	sockets.broadcast<false>(no::packet_stream(new_packet), client_index);
	sockets[client_index].send(no::packet_stream(new_packet));
	persister.save_player_tile(client.player.id, new_packet.tile);
}

void server_state::on_start_dialogue(int client_index, const to_server::game::start_dialogue& packet) {
	auto& client = clients[client_index];
	if (client.dialogue) {
		WARNING("Player is already in a dialogue");
		return;
	}
	auto target = worlds.front().objects.find(packet.target_instance_id);
	if (!target) {
		WARNING("Target " << packet.target_instance_id << " not found.");
		return;
	}
	auto player = worlds.front().objects.find(client.object.player_instance_id);
	if (!player) {
		WARNING("Player not found");
		return;
	}
	int dialogue_id = 0; // todo: associate dialogues with objects
	client.dialogue = new dialogue_tree();
	client.dialogue->variables = &client.object.variables;
	client.dialogue->player = (character_object*)player;
	client.dialogue->inventory = &client.dialogue->player->inventory;
	client.dialogue->equipment = &client.dialogue->player->equipment;
	client.dialogue->load(dialogue_id);
	client.dialogue->process_entry_point();
	persister.save_player_variables(client.player.id, client.object.variables);
}

void server_state::on_continue_dialogue(int client_index, const to_server::game::continue_dialogue& packet) {
	auto& client = clients[client_index];
	if (!client.dialogue) {
		WARNING("Player is not in a dialogue");
		return;
	}
	client.dialogue->select_choice(packet.choice);
	persister.save_player_variables(client.player.id, client.object.variables);
}

void server_state::on_chat_message(int client_index, const to_server::game::chat_message& packet) {
	to_client::game::chat_message client_packet;
	client_packet.author = clients[client_index].player.display_name;
	client_packet.message = packet.message;
	sockets.broadcast<true>(no::packet_stream(client_packet), client_index);
}

void server_state::on_login_attempt(int client_index, const to_server::lobby::login_attempt& packet) {
	auto result = database.execute("select * from player where account_email = $1", { packet.name });
	if (result.count() != 1) {
		return;
	}
	auto row = result.row(0);
	auto& client = clients[client_index];
	client.account.email = row.text("account_email");
	client.player.id = row.integer("id");
	client.player.display_name = row.text("display_name");
	to_client::lobby::login_status client_packet;
	client_packet.status = 1;
	client_packet.name = client.player.display_name;
	sockets[client_index].send(no::packet_stream(client_packet));
}

void server_state::on_connect_to_world(int client_index, const to_server::lobby::connect_to_world& packet) {
	auto player = load_player(client_index);

	to_client::game::my_player_info my_info;
	my_info.player = *player;
	my_info.variables = clients[client_index].object.variables;
	sockets[client_index].send(no::packet_stream(my_info));

	for (int j = 0; j < max_clients; j++) {
		if (clients[j].is_connected() && client_index != j) {
			auto existing_player = worlds.front().objects.find(clients[j].object.player_instance_id);
			if (!existing_player) {
				WARNING("Player not found");
				continue;
			}
			to_client::game::other_player_joined other_info;
			other_info.player = *(character_object*)existing_player;
			sockets[client_index].send(no::packet_stream(other_info));
		}
	}

	to_client::game::other_player_joined other_info;
	other_info.player = *(character_object*)worlds.front().objects.find(clients[client_index].object.player_instance_id);
	sockets.broadcast<false>(no::packet_stream(other_info), client_index);
}

void server_state::on_version_check(int client_index, const to_server::updates::update_query& packet) {
	if (packet.version != newest_client_version || packet.needs_assets) {
		updaters.emplace_back(sockets[client_index]);
	}
	auto new_packet = packet;
	new_packet.version = newest_client_version;
	sockets[client_index].send(no::packet_stream(new_packet));
}

