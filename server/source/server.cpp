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
	for (auto& fisher : fishers) {
		if (fisher.last_progress.seconds() < 2 || fisher.finished) {
			continue;
		}
		auto& object = objects.object(fisher.player_instance_id);
		no::vector2i tile = object.tile();
		bool left = (tile.x < fisher.bait_tile.x);
		bool right = (tile.x > fisher.bait_tile.x);
		bool up = (tile.y < fisher.bait_tile.y);
		bool down = (tile.y > fisher.bait_tile.y);
		fisher.bait_tile.x += (left ? -1 : (right ? 1 : 0));
		fisher.bait_tile.y += (up ? -1 : (down ? 1 : 0));
		if (fisher.bait_tile == tile) {
			fisher.finished = true;
		}
		if (terrain.tiles().at(fisher.bait_tile.x, fisher.bait_tile.y).corners_of_type(world_tile::water) < 3) {
			fisher.finished = true;
		}
		fisher.last_progress.start();
	}
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
		if (definition.script_id.killed >= 0) {
			for (auto& client : clients) {
				if (client.object.player_instance_id == event.attacker_id) {
					auto player = world.objects.character(event.attacker_id);
					script_tree script;
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
	for (int i = 0; i < (int)world.fishers.size(); i++) {
		auto& fisher = world.fishers[i];
		if (fisher.fished_for.seconds() % 2 == 1) {
			continue;
		}
		auto& client = clients[fisher.client_index];
		to_client::game::fishing_progress fishing_progress;
		fishing_progress.new_bait_tile = fisher.bait_tile;
		fishing_progress.instance_id = client.object.player_instance_id;
		if (fisher.finished) {
			auto character = world.objects.character(client.object.player_instance_id);
			item_instance fish{ 10, 1 };
			character->inventory.add_from(fish);
			to_client::game::fish_caught fish_caught;
			fish_caught.item = { 10, 1 };
			no::send_packet(fisher.client_index, fish_caught);
			fishing_progress.finished = true;
			world.fishers.erase(world.fishers.begin() + i);
			i--;
		}
		no::broadcast(fishing_progress);
	}
}

void server_state::draw() {
	
}

int server_state::client_with_player(int player_id) {
	for (int i = 0; i < max_clients; i++) {
		if (clients[i].object.player_instance_id == player_id) {
			return i;
		}
	}
	return -1;
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
	player->name = client.player.display_name;
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
#define OnPacket(NS, PACKET) case to_server::NS::PACKET::type: on_##PACKET(client_index, { stream }); break
	switch (type) {
		OnPacket(game, move_to_tile);
		OnPacket(game, start_dialogue);
		OnPacket(game, continue_dialogue);
		OnPacket(game, chat_message);
		OnPacket(game, start_combat);
		OnPacket(game, equip_from_inventory);
		OnPacket(game, unequip_to_inventory);
		OnPacket(game, follow_character);
		OnPacket(game, trade_request);
		OnPacket(game, add_trade_item);
		OnPacket(game, remove_trade_item);
		OnPacket(game, trade_decision);
		OnPacket(game, started_fishing);
		OnPacket(game, consume_from_inventory);
		OnPacket(lobby, login_attempt);
		OnPacket(lobby, connect_to_world);
		OnPacket(updates, update_query);
	}
#undef OnPacket
}

void server_state::on_disconnect(int client_index) {
	remove_trade(client_index);
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
	no::broadcast(new_packet);

	// todo: this should be its own function, just testing now
	to_client::game::character_follows client_packet;
	client_packet.follower_id = client.object.player_instance_id;
	client_packet.target_id = -1;
	no::broadcast(client_packet);
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
	client.dialogue = new script_tree;
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

void server_state::on_trade_request(int client_index, const to_server::game::trade_request& packet) {
	auto& client = clients[client_index];
	if (find_trade(client_index)) {
		return;
	}
	int target_client_index = client_with_player(packet.trade_with_id);
	if (target_client_index == -1) {
		return;
	}
	auto& target_client = clients[target_client_index];
	if (target_client.sent_trade_request_to_player_id == client.object.player_instance_id) {
		auto& trade = trades.emplace_back();
		trade.first_client_index = client_index;
		trade.second_client_index = target_client_index;
		to_client::game::trade_request trade_request;
		trade_request.trader_id = target_client.object.player_instance_id;
		no::send_packet(client_index, trade_request);
	}
	client.sent_trade_request_to_player_id = target_client.object.player_instance_id;
	to_client::game::trade_request trade_request;
	trade_request.trader_id = client.object.player_instance_id;
	no::send_packet(target_client_index, trade_request);
}

void server_state::on_add_trade_item(int client_index, const to_server::game::add_trade_item& packet) {
	auto& client = clients[client_index];
	item_instance item{ packet.item };
	auto trade = find_trade(client_index);
	if (!trade) {
		return;
	}
	to_client::game::add_trade_item add_trade_item;
	add_trade_item.item = item;
	if (trade->first_client_index == client_index) {
		trade->first.add_from(item);
		no::send_packet(trade->second_client_index, add_trade_item);
	} else if (trade->second_client_index == client_index) {
		trade->second.add_from(item);
		no::send_packet(trade->first_client_index, add_trade_item);
	}
	trade->first_accepts = false;
	trade->second_accepts = false;
}

void server_state::on_remove_trade_item(int client_index, const to_server::game::remove_trade_item& packet) {
	auto& client = clients[client_index];
	auto trade = find_trade(client_index);
	if (!trade) {
		return;
	}
	to_client::game::remove_trade_item remove_trade_item;
	remove_trade_item.slot = packet.slot;
	item_instance temp;
	if (trade->first_client_index == client_index) {
		trade->first.remove_to(trade->first.get(packet.slot).stack, temp);
		no::send_packet(trade->second_client_index, remove_trade_item);
	} else if (trade->second_client_index == client_index) {
		trade->second.remove_to(trade->second.get(packet.slot).stack, temp);
		no::send_packet(trade->first_client_index, remove_trade_item);
	}
	trade->first_accepts = false;
	trade->second_accepts = false;
}

void server_state::on_trade_decision(int client_index, const to_server::game::trade_decision& packet) {
	auto& client = clients[client_index];
	auto trade = find_trade(client_index);
	if (!trade) {
		return;
	}
	to_client::game::trade_decision trade_decision;
	trade_decision.accepted = packet.accepted;
	if (trade->first_client_index == client_index) {
		trade->first_accepts = packet.accepted;
		no::send_packet(trade->second_client_index, trade_decision);
	} else if (trade->second_client_index == client_index) {
		trade->second_accepts = packet.accepted;
		no::send_packet(trade->first_client_index, trade_decision);
	}
	if (!packet.accepted) {
		remove_trade(client_index);
	} else if (trade->first_accepts && trade->second_accepts) {
		auto& first_client = clients[trade->first_client_index];
		auto& second_client = clients[trade->second_client_index];
		auto first_player = world.objects.character(first_client.object.player_instance_id);
		auto second_player = world.objects.character(second_client.object.player_instance_id);
		for (auto& item : trade->first.items) {
			item_instance temp;
			temp.definition_id = item.definition_id;
			first_player->inventory.remove_to(item.stack, temp);
			second_player->inventory.add_from(temp);
		}
		for (auto& item : trade->second.items) {
			item_instance temp;
			temp.definition_id = item.definition_id;
			second_player->inventory.remove_to(item.stack, temp);
			first_player->inventory.add_from(temp);
		}
		remove_trade(client_index);
	}
}

void server_state::on_started_fishing(int client_index, const to_server::game::started_fishing& packet) {
	auto& client = clients[client_index];
	auto player = world.objects.character(client.object.player_instance_id);
	if (!player) {
		return;
	}
	if (player->is_fishing()) {
		return;
	}
	auto object = world.objects.object(client.object.player_instance_id);
	if (!world.can_fish_at(object.tile(), packet.casted_to_tile)) {
		return;
	}
	auto& fisher = world.fishers.emplace_back();
	fisher.fished_for.start();
	fisher.last_progress.start();
	fisher.bait_tile = packet.casted_to_tile;
	fisher.client_index = client_index;
	fisher.player_instance_id = client.object.player_instance_id;
	player->events.start_fishing.emit();
	to_client::game::started_fishing started_fishing;
	started_fishing.instance_id = client.object.player_instance_id;
	started_fishing.casted_to_tile = packet.casted_to_tile;
	no::broadcast(started_fishing);
}

void server_state::on_consume_from_inventory(int client_index, const to_server::game::consume_from_inventory& packet) {
	auto& client = clients[client_index];
	auto character = world.objects.character(client.object.player_instance_id);
	auto item = character->inventory.get(packet.slot);
	if (item.definition().type != item_type::consumable) {
		return;
	}
	character->consume_from_inventory(packet.slot);
}

void server_state::on_login_attempt(int client_index, const to_server::lobby::login_attempt& packet) {
	auto result = database.execute("select * from player where account_email = $1", { packet.name });
	if (result.count() != 1) {
		return;
	}
	auto row = result.row(0);
	for (auto& client : clients) {
		if (client.account.email == packet.name) {
			to_client::lobby::login_status login_status;
			login_status.status = 2;
			no::send_packet(client_index, login_status);
			return;
		}
	}
	auto& client = clients[client_index];
	client.account.email = row.text("account_email");
	client.player.id = row.integer("id");
	client.player.display_name = row.text("display_name");
	to_client::lobby::login_status login_status;
	login_status.status = 1;
	login_status.name = client.player.display_name;
	no::send_packet(client_index, login_status);
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
			other_info.player = *existing_player;
			other_info.object = world.objects.object(other_info.player.object_id);
			no::send_packet(client_index, other_info);
		}
	}

	to_client::game::other_player_joined other_info;
	other_info.player = *world.objects.character(clients[client_index].object.player_instance_id);
	other_info.object = world.objects.object(other_info.player.object_id);
	no::broadcast(other_info, client_index);
}

void server_state::on_update_query(int client_index, const to_server::updates::update_query& packet) {
	if (packet.version != newest_client_version || packet.needs_assets) {
		updaters.emplace_back(client_index);
	}
	to_client::updates::latest_version latest_version;
	latest_version.version = newest_client_version;
	no::send_packet(client_index, latest_version);
}

trade_state* server_state::find_trade(int client_index) {
	for (auto& trade : trades) {
		if (trade.first_client_index == client_index || trade.second_client_index == client_index) {
			return &trade;
		}
	}
	return nullptr;
}

void server_state::remove_trade(int client_index) {
	for (int i = 0; i < (int)trades.size(); i++) {
		if (trades[i].first_client_index == client_index || trades[i].second_client_index == client_index) {
			trades.erase(trades.begin() + i);
			return;
		}
	}
}
