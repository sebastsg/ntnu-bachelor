#include "game.hpp"
#include "window.hpp"
#include "platform.hpp"
#include "packets.hpp"
#include "network.hpp"
#include "hit_splats.hpp"
#include "game_assets.hpp"
#include "ui_trading.hpp"
#include "ui_context.hpp"
#include "ui_dialogue.hpp"
#include "ui_tabs.hpp"
#include "ui_hud.hpp"
#include "chat.hpp"
#include "pathfinding.hpp"

game_world::game_world() {
	name = "main";
	objects.load();
	terrain.load(1);
}

player_data game_world::my_player() {
	return player_data{
		*objects.character(my_player_id),
		objects.object(my_player_id)
	};
}

game_state::game_state() : renderer(world), dragger(mouse()) {
	ui_camera.zoom = 2.0f;
	create_game_assets();
	start_hit_splats(*this);
	show_tabs(*this);
	show_hud(*this);
	mouse_press_id = mouse().press.listen([this](const no::mouse::press_message& event) {
		if (event.button != no::mouse::button::left) {
			return;
		}
		if (is_mouse_over_tabs() || is_trading() || is_mouse_over_context_menu()) {
			return;
		}
		no::vector2i tile = hovered_pixel.xy;
		if (tile.x != -1) {
			to_server::game::move_to_tile packet;
			packet.tile = tile;
			no::send_packet(server(), packet);
		}
	});

	mouse_scroll_id = mouse().scroll.listen([this](const no::mouse::scroll_message& event) {
		float& factor = renderer.camera.rotation_offset_factor;
		factor -= (float)event.steps;
		if (factor > 12.0f) {
			factor = 12.0f;
		} else if (factor < 4.0f) {
			factor = 4.0f;
		}
	});

	keyboard_press_id = keyboard().press.listen([this](const no::keyboard::press_message& event) {
		if (event.key == no::key::num_7) {
			renderer.reload_shaders();
		}
	});

	show_chat(*this);
	chat_message_event().listen([this](const std::string& message) {
		to_server::game::chat_message packet;
		packet.message = message;
		no::send_packet(server(), packet);
	});

	receive_packet_id = no::socket_event(server()).packet.listen([this](const no::io_stream& packet) {
		no::io_stream stream{ packet.data(), packet.size(), no::io_stream::construct_by::shallow_copy };
		int16_t type = stream.read<int16_t>();
		switch (type) {
		case to_client::game::move_to_tile::type:
		{
			to_client::game::move_to_tile packet{ stream };
			auto player = world.objects.character(packet.player_instance_id);
			if (player) {
				auto& object = world.objects.object(packet.player_instance_id);
				auto path = world.path_between(object.tile(), packet.tile);
				if (!path.empty() && object.tile() == path.back()) {
					path.pop_back();
				}
				player->start_path_movement(path);
			} else {
				WARNING("player not found: " << packet.player_instance_id);
			}
			break;
		}
		case to_client::game::my_player_info::type:
		{
			to_client::game::my_player_info packet{ stream };
			no::io_stream objstream;
			packet.object.write(objstream);
			packet.player.write(objstream);
			world.objects.add(objstream);
			world.my_player_id = packet.object.instance_id;
			world.my_player().object.pickable = false;
			world.my_player().character.running = true;
			enable_tabs();
			variables = packet.variables;
			quests = packet.quests;
			break;
		}
		case to_client::game::other_player_joined::type:
		{
			to_client::game::other_player_joined packet{ stream };
			packet.player.running = true;
			no::io_stream objstream;
			packet.object.write(objstream);
			packet.player.write(objstream);
			world.objects.add(objstream);
			break;
		}
		case to_client::game::chat_message::type:
		{
			to_client::game::chat_message packet{ stream };
			add_chat_message(packet.author, packet.message);
			break;
		}
		case to_client::game::combat_hit::type:
		{
			to_client::game::combat_hit packet{ stream };
			add_hit_splat(packet.target_id, packet.damage);
			auto attacker = world.objects.character(packet.attacker_id);
			auto target = world.objects.character(packet.target_id);
			target->stat(stat_type::health).add_effective(-packet.damage);
			attacker->add_combat_experience(packet.damage);
			attacker->events.attack.emit();
			target->events.defend.emit();
			target->target_path.clear();
			auto& attacker_object = world.objects.object(attacker->object_id);
			auto& target_object = world.objects.object(target->object_id);
			attacker_object.transform.rotation.y = angle_to_goal(attacker_object.transform.position, target_object.transform.position);
			target_object.transform.rotation.y = angle_to_goal(target_object.transform.position, attacker_object.transform.position);
			if (attacker->stat(stat_type::health).effective() < 1) {
				world.objects.remove(packet.attacker_id);
			}
			if (target->stat(stat_type::health).effective() < 1) {
				world.objects.remove(packet.target_id);
			}
			break;
		}
		case to_client::game::character_equips::type:
		{
			to_client::game::character_equips packet{ stream };
			auto character = world.objects.character(packet.instance_id);
			if (character) {
				character->equip({ packet.item_id, packet.stack });
			}
			break;
		}
		case to_client::game::update_character_path::type:
		{
			to_client::game::update_character_path packet{ stream };
			auto character = world.objects.character(packet.instance_id);
			if (character) {
				character->start_path_movement(packet.path);
			}
			break;
		}
		case to_client::game::trade_request::type:
		{
			to_client::game::trade_request packet{ stream };
			if (sent_trade_request_to_player_id == packet.trader_id) {
				start_trading(*this, packet.trader_id);
			} else {
				auto trader = world.objects.character(packet.trader_id);
				if (trader) {
					add_chat_message("", trader->name + " wants to trade with you.");
				}
			}
			break;
		}
		case to_client::game::add_trade_item::type:
		{
			to_client::game::add_trade_item packet{ stream };
			add_trade_item(0, packet.item);
			break;
		}
		case to_client::game::remove_trade_item::type:
		{
			to_client::game::remove_trade_item packet{ stream };
			remove_trade_item(0, packet.slot);
			break;
		}
		case to_client::game::trade_decision::type:
		{
			to_client::game::trade_decision packet{ stream };
			notify_trade_decision(packet.accepted);
			sent_trade_request_to_player_id = -1;
			break;
		}
		case to_client::game::started_fishing::type:
		{
			to_client::game::started_fishing packet{ stream };
			auto character = world.objects.character(packet.instance_id);
			if (character) {
				character->bait_tile = packet.casted_to_tile;
				character->events.start_fishing.emit();
			}
			break;
		}
		case to_client::game::fishing_progress::type:
		{
			to_client::game::fishing_progress packet{ stream };
			auto character = world.objects.character(packet.instance_id);
			if (character) {
				character->bait_tile = packet.new_bait_tile;
				if (packet.finished) {
					character->events.stop_fishing.emit();
				}
			}
			break;
		}
		case to_client::game::fish_caught::type:
		{
			to_client::game::fish_caught packet{ stream };
			auto& player = world.my_player().character;
			player.inventory.add_from(packet.item);
			player.stat(stat_type::fishing).add_experience(270);
			break;
		}
		case to_client::game::rotate_object::type:
		{
			to_client::game::rotate_object packet{ stream };
			auto& object = world.objects.object(packet.instance_id);
			object.transform.rotation = packet.rotation;
			break;
		}
		default:
			break;
		}
	});
	window().set_clear_color({ 160.0f / 255.0f, 230.0f / 255.0f, 1.0f });
}

game_state::~game_state() {
	hide_chat();
	close_dialogue();
	stop_hit_splats();
	hide_tabs();
	hide_hud();
	mouse().press.ignore(mouse_press_id);
	mouse().scroll.ignore(mouse_scroll_id);
	keyboard().press.ignore(keyboard_press_id);
	no::socket_event(server()).packet.ignore(receive_packet_id);
	destroy_game_assets();
}

void game_state::update() {
	no::synchronize_socket(server());
	renderer.camera.size = window().size().to<float>();
	ui_camera.transform.scale = window().size().to<float>();
	if (world.my_player_id == -1) {
		return;
	}
	no::transform3 camera_follow = world.my_player().object.transform;
	camera_follow.position.y = 0.0f; // since it's annoying if camera bumps up and down
	follower.update(renderer.camera, camera_follow);
	dragger.update(renderer.camera);
	rotater.update(renderer.camera, keyboard());
	world.update();
	renderer.camera.update();

	set_hud_fps(frame_counter().current_fps());
	set_hud_debug(STRING("Tile: " << world.my_player().object.tile()));
	update_tabs();
	update_hud();
	update_trading_ui();
	update_hit_splats();
	update_chat();
	update_dialogue();
	no::vector2i previous_offset = world.terrain.offset();
	world.terrain.shift_to_center_of(world.my_player().object.tile());
	if (previous_offset != world.terrain.offset()) {
		renderer.update_object_visibility();
	}
}

void game_state::draw() {
	if (world.my_player_id == -1) {
		return;
	}
	renderer.draw_for_picking();
	hovered_pixel = no::read_pixel_at({ mouse().x(), window().height() - mouse().y() });
	hovered_pixel.x--;
	hovered_pixel.y--;
	hovered_pixel.xy += world.terrain.offset();
	window().clear();
	renderer.light.position.x = world.my_player().object.transform.position.x;
	renderer.light.position.y = 32.0f;
	renderer.light.position.z = world.my_player().object.transform.position.z;
	renderer.draw();
	if (world.my_player_id != -1) {
		if (hovered_pixel.x != -1 && !is_context_menu_open()) {
			renderer.draw_tile_highlights({ hovered_pixel.xy }, { 0.3f, 0.4f, 0.8f, 0.4f });
			if (keyboard().is_key_down(no::key::num_8)) {
				path_found = world.path_between(world.my_player().object.tile(), hovered_pixel.xy);
			}
			renderer.draw_tile_highlights(path_found, { 1.0f, 1.0f, 0.2f, 0.8f });
		}
		if (world.my_player().character.is_fishing()) {
			std::vector<no::vector2i> bait_tiles;
			bait_tiles.push_back(world.my_player().character.bait_tile);
			renderer.draw_tile_highlights(bait_tiles, { 1.0f, 0.0f, 0.5f, 0.5f });
		}
	}

	no::bind_shader(shaders().sprite.id);
	no::set_shader_view_projection(ui_camera);
	shaders().sprite.color.set(no::vector4f{ 1.0f });
	no::bind_texture(sprites().ui);

	draw_hud();
	draw_tabs();
	draw_hit_splats();
	draw_chat();
	draw_dialogue();
}

no::vector2i game_state::hovered_tile() const {
	return hovered_pixel.xy;
}

const no::perspective_camera& game_state::world_camera() const {
	return renderer.camera;
}

void game_state::start_dialogue(int target_id) {
	to_server::game::start_dialogue packet;
	packet.target_instance_id = target_id;
	no::send_packet(server(), packet);
	open_dialogue(*this, target_id);
	dialogue_choice_event().listen([this](int choice) {
		to_server::game::continue_dialogue packet;
		packet.choice = choice;
		no::send_packet(server(), packet);
	});
}

void game_state::start_combat(int target_id) {
	to_server::game::start_combat packet;
	packet.target_id = target_id;
	no::send_packet(server(), packet);
	follow_character(target_id);
}

void game_state::follow_character(int target_id) {
	to_server::game::follow_character packet;
	packet.target_id = target_id;
	no::send_packet(server(), packet);
}

void game_state::equip_from_inventory(no::vector2i slot) {
	world.my_player().character.equip_from_inventory(slot);
	to_server::game::equip_from_inventory packet;
	packet.slot = slot;
	no::send_packet(server(), packet);
}

void game_state::unequip_to_inventory(equipment_slot slot) {
	world.my_player().character.unequip_to_inventory(slot);
	to_server::game::unequip_to_inventory packet;
	packet.slot = slot;
	no::send_packet(server(), packet);
}

void game_state::consume_from_inventory(no::vector2i slot) {
	world.my_player().character.consume_from_inventory(slot);
	to_server::game::consume_from_inventory packet;
	packet.slot = slot;
	no::send_packet(server(), packet);
}

void game_state::send_trade_request(int target_id) {
	sent_trade_request_to_player_id = target_id;
	to_server::game::trade_request packet;
	packet.trade_with_id = target_id;
	no::send_packet(server(), packet);
}

void game_state::send_add_trade_item(const item_instance& item) {
	to_server::game::add_trade_item packet;
	packet.item = item;
	no::send_packet(server(), packet);
	add_trade_item(1, item);
}

void game_state::send_remove_trade_item(no::vector2i slot) {
	to_server::game::remove_trade_item packet;
	packet.slot = slot;
	no::send_packet(server(), packet);
	remove_trade_item(1, slot);
}

void game_state::send_finish_trading(bool accept) {
	to_server::game::trade_decision packet;
	packet.accepted = accept;
	no::send_packet(server(), packet);
}

void game_state::send_start_fishing(no::vector2i tile) {
	to_server::game::started_fishing packet;
	packet.casted_to_tile = tile;
	no::send_packet(server(), packet);
}
