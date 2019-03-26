#include "server.hpp"
#include "debug.hpp"
#include "io.hpp"
#include "platform.hpp"
#include "assets.hpp"
#include "packets.hpp"

server_world::server_world(const std::string& name) : combat(*this) {
	load(no::asset_path("worlds/" + name + ".ew"));
}

void server_world::update() {
	world_state::update();
	combat.update();
}

server_state::server_state() : persister(database), world("main") {
	listener = no::open_socket();
	no::bind_socket(listener, "10.0.0.130", 7524); // todo: config file
	no::listen_socket(listener);
	no::socket_event(listener).accept.listen([this](int accepted_id) {
		connect(accepted_id);
	});
	combat_hit_event_id = world.combat.events.hit.listen([this](const combat_system::hit_event& event) {
		to_client::game::combat_hit packet;
		packet.attacker_id = event.attacker_id;
		packet.target_id = event.target_id;
		packet.damage = event.damage;
		no::broadcast(packet);
	});
}

server_state::~server_state() {
	for (int i = 0; i < max_clients; i++) {
		save_player(i);
	}
	world.combat.events.hit.ignore(combat_hit_event_id);
}

void server_state::update() {
	no::synchronise_sockets();
	for (int i = 0; i < (int)updaters.size(); i++) {
		updaters[i].update();
		if (updaters[i].is_done()) {
			updaters.erase(updaters.begin() + i);
			i--;
		}
	}
	for (int i = 0; i < max_clients; i++) {
		if (clients[i].is_connected() && clients[i].last_saved.seconds() > 120) {
			save_player(i);
		}
	}
	world.update();
	world.events.kill.all([&](const server_world::kill_event& event) {
		const auto& definition = world.objects.object(event.target_id).definition();
		if (definition.script_id.killed) {
			for (auto& client : clients) {
				if (client.object.player_instance_id == event.attacker_id) {
					auto player = world.objects.character(event.attacker_id);
					dialogue_tree script;
					script.quests = &client.player.quests;
					script.variables = &client.player.variables;
					script.player_object_id = client.object.player_instance_id;
					script.inventory = &player->inventory;
					script.equipment = &player->equipment;
					script.world = &world;
					script.load(definition.script_id.killed);
					script.process_entry_point();
					break;
				}
			}
		}
		world.combat.stop_all(event.target_id);
		world.objects.remove(event.target_id);
	});
}

void server_state::draw() {
	
}

character_object* server_state::load_player(int client_index) {
	auto& client = clients[client_index];
	client.object.player_instance_id = world.objects.add(1);
	auto player = world.objects.character(client.object.player_instance_id);
	no::vector2i tile = persister.load_player_tile(client.player.id);
	client.player.variables = persister.load_player_variables(client.player.id);
	client.player.quests = persister.load_player_quests(client.player.id);
	client.object.player_instance_id = player->object_id;
	persister.load_player_items(client.player.id, inventory_container_type, player->inventory.items, player->inventory.slots);
	persister.load_player_items(client.player.id, equipment_container_type, player->equipment.items, (int)equipment_slot::total_slots);
	auto& object = world.objects.object(client.object.player_instance_id);
	object.transform.position.x = (float)tile.x;
	object.transform.position.z = (float)tile.y;
	object.transform.scale = 0.5f;
	player->stat(stat_type::health).add_experience(player->stat(stat_type::health).experience_for_level(20));
	return player;
}

void server_state::save_player(int client_index) {
	auto& client = clients[client_index];
	if (!client.is_connected()) {
		return;
	}
	auto player = world.objects.character(client.object.player_instance_id);
	if (!player) {
		WARNING("Failed to save player");
		return;
	}
	auto& object = world.objects.object(client.object.player_instance_id);
	persister.save_player_tile(client.player.id, object.tile());
	persister.save_player_variables(client.player.id, client.player.variables);
	persister.save_player_quests(client.player.id, client.player.quests);
	persister.save_player_items(client.player.id, inventory_container_type, player->inventory.items, player->inventory.slots);
	persister.save_player_items(client.player.id, equipment_container_type, player->equipment.items, (int)equipment_slot::total_slots);
	client.last_saved.start();
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
	no::socket_event(index).packet.listen([this, index](const no::io_stream& packet) {
		no::io_stream stream{ packet.data(), packet.size(), no::io_stream::construct_by::shallow_copy };
		int16_t type = stream.read<int16_t>();
		on_receive_packet(index, type, stream);
	});
	no::socket_event(index).disconnect.listen([this, index](const no::socket_close_status& status) {
		INFO("Client " << index << " has disconnected with status " << status);
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
	case to_server::game::start_combat::type:
		on_start_combat(client_index, { stream });
		break;
	case to_server::game::equip_from_inventory::type:
		on_equip_from_inventory(client_index, { stream });
		break;
	case to_server::game::unequip_to_inventory::type:
		on_unequip_to_inventory(client_index, { stream });
		break;
	case to_server::game::follow_character::type:
		on_follow_character(client_index, { stream });
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
	save_player(client_index);
	world.objects.remove(clients[client_index].object.player_instance_id);
	clients[client_index] = { false };
	to_client::game::player_disconnected disconnection;
	disconnection.player_instance_id = clients[client_index].object.player_instance_id;
	no::broadcast(disconnection);
}

void server_state::on_move_to_tile(int client_index, const to_server::game::move_to_tile& packet) {
	auto& client = clients[client_index];
	to_client::game::move_to_tile new_packet;
	new_packet.tile = packet.tile;
	new_packet.player_instance_id = client.object.player_instance_id;
	auto player = world.objects.character(new_packet.player_instance_id);
	if (player) {
		// todo: pathfinding
		auto& object = world.objects.object(player->object_id);
		object.transform.position.x = (float)packet.tile.x;
		object.transform.position.z = (float)packet.tile.y;
	}
	no::broadcast(new_packet, client_index);
	no::send_packet(client_index, new_packet);

	// todo: this should be its own function, just testing now
	to_client::game::character_follows client_packet;
	client_packet.follower_id = client.object.player_instance_id;
	client_packet.target_id = -1;
	no::broadcast(client_packet, client_index);
	no::send_packet(client_index, client_packet);
}

void server_state::on_start_dialogue(int client_index, const to_server::game::start_dialogue& packet) {
	auto& client = clients[client_index];
	if (client.dialogue) {
		WARNING("Player is already in a dialogue");
		return;
	}
	auto target = world.objects.character(packet.target_instance_id);
	if (!target) {
		WARNING("Target " << packet.target_instance_id << " not found.");
		return;
	}
	auto player = world.objects.character(client.object.player_instance_id);
	if (!player) {
		WARNING("Player not found");
		return;
	}
	int dialogue_id = 0; // todo: associate dialogues with objects
	client.dialogue = new dialogue_tree();
	client.dialogue->quests = &client.player.quests;
	client.dialogue->variables = &client.player.variables;
	client.dialogue->player_object_id = client.object.player_instance_id;
	client.dialogue->inventory = &player->inventory;
	client.dialogue->equipment = &player->equipment;
	client.dialogue->world = &world;
	client.dialogue->load(dialogue_id);
	client.dialogue->process_entry_point();
}

void server_state::on_continue_dialogue(int client_index, const to_server::game::continue_dialogue& packet) {
	auto& client = clients[client_index];
	if (!client.dialogue) {
		WARNING("Player is not in a dialogue");
		return;
	}
	client.dialogue->select_choice(packet.choice);
}

void server_state::on_chat_message(int client_index, const to_server::game::chat_message& packet) {
	to_client::game::chat_message client_packet;
	client_packet.author = clients[client_index].player.display_name;
	client_packet.message = packet.message;
	no::broadcast(client_packet, client_index);
}

void server_state::on_start_combat(int client_index, const to_server::game::start_combat& packet) {
	world.combat.add(clients[client_index].object.player_instance_id, packet.target_id);
}

void server_state::on_equip_from_inventory(int client_index, const to_server::game::equip_from_inventory& packet) {
	auto& client = clients[client_index];
	auto character = world.objects.character(client.object.player_instance_id);
	auto item = character->inventory.get(packet.slot);
	character->equip_from_inventory(packet.slot);
	to_client::game::character_equips client_packet;
	client_packet.instance_id = client.object.player_instance_id;
	client_packet.item_id = item.definition_id;
	client_packet.stack = item.stack;
	no::broadcast(client_packet, client_index);
}

void server_state::on_unequip_to_inventory(int client_index, const to_server::game::unequip_to_inventory& packet) {
	auto& client = clients[client_index];
	auto character = world.objects.character(client.object.player_instance_id);
	auto item = character->equipment.get(packet.slot);
	character->unequip_to_inventory(packet.slot);
	to_client::game::character_unequips client_packet;
	client_packet.instance_id = client.object.player_instance_id;
	client_packet.slot = packet.slot;
	no::broadcast(client_packet, client_index);
}

void server_state::on_follow_character(int client_index, const to_server::game::follow_character& packet) {
	auto& client = clients[client_index];
	auto follower = world.objects.character(client.object.player_instance_id);
	if (follower) {
		follower->follow_object_id = packet.target_id;
		to_client::game::character_follows client_packet;
		client_packet.follower_id = follower->object_id;
		client_packet.target_id = packet.target_id;
		no::broadcast(client_packet, client_index);
		no::send_packet(client_index, client_packet);
	}
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
	no::send_packet(client_index, client_packet);
}

void server_state::on_connect_to_world(int client_index, const to_server::lobby::connect_to_world& packet) {
	auto player = load_player(client_index);

	to_client::game::my_player_info my_info;
	my_info.player = *player;
	my_info.object = world.objects.object(player->object_id);
	my_info.variables = clients[client_index].player.variables;
	my_info.quests = clients[client_index].player.quests;
	no::send_packet(client_index, my_info);

	for (int j = 0; j < max_clients; j++) {
		if (clients[j].is_connected() && client_index != j) {
			auto existing_player = world.objects.character(clients[j].object.player_instance_id);
			if (!existing_player) {
				WARNING("Player not found");
				continue;
			}
			to_client::game::other_player_joined other_info;
			other_info.player = *(character_object*)existing_player;
			no::send_packet(client_index, other_info);
		}
	}

	to_client::game::other_player_joined other_info;
	other_info.player = *world.objects.character(clients[client_index].object.player_instance_id);
	other_info.object = world.objects.object(other_info.player.object_id);
	no::broadcast(other_info, client_index);
}

void server_state::on_version_check(int client_index, const to_server::updates::update_query& packet) {
	if (packet.version != newest_client_version || packet.needs_assets) {
		updaters.emplace_back(client_index);
	}
	to_client::updates::latest_version latest_version;
	latest_version.version = newest_client_version;
	no::send_packet(client_index, latest_version);
}
