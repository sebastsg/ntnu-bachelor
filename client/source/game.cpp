#include "game.hpp"
#include "window.hpp"
#include "debug.hpp"
#include "surface.hpp"
#include "io.hpp"
#include "platform.hpp"
#include "commands.hpp"
#include "packets.hpp"
#include "network.hpp"
#include "pathfinding.hpp"

game_world::game_world() {
	load(no::asset_path("worlds/main.ew"));
}

player_data game_world::my_player() {
	return player_data{
		*objects.character(my_player_id),
		objects.object(my_player_id)
	};
}

game_state::game_state() : 
	renderer(world), 
	dragger(mouse()), 
	ui(*this, world), 
	chat(*this, ui.camera), 
	ui_font(no::asset_path("fonts/leo.ttf"), 14) {
	mouse_press_id = mouse().press.listen([this](const no::mouse::press_message& event) {
		if (event.button != no::mouse::button::left) {
			return;
		}
		if (ui.is_mouse_over_any()) {
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
		
	});

	chat.events.message.listen([this](const chat_view::message_event& event) {
		to_server::game::chat_message packet;
		packet.message = event.message;
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
				player->start_path_movement(world.path_between(object.tile(), packet.tile));
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
			ui.listen(world.my_player_id);
			variables = packet.variables;
			quests = packet.quests;
			break;
		}
		case to_client::game::other_player_joined::type:
		{
			to_client::game::other_player_joined packet{ stream };
			no::io_stream objstream;
			packet.object.write(objstream);
			packet.player.write(objstream);
			world.objects.add(objstream);
			break;
		}
		case to_client::game::chat_message::type:
		{
			to_client::game::chat_message packet{ stream };
			chat.add(packet.author, packet.message);
			break;
		}
		case to_client::game::combat_hit::type:
		{
			to_client::game::combat_hit packet{ stream };
			ui.hud.hit_splats.emplace_back(*this, packet.target_id, packet.damage);
			auto attacker = world.objects.character(packet.attacker_id);
			auto target = world.objects.character(packet.target_id);
			target->stat(stat_type::health).add_effective(-packet.damage);
			attacker->events.attack.emit();
			target->events.defend.emit();
			attacker->follow_object_id = target->object_id;
			target->follow_object_id = attacker->object_id;
			attacker->follow_distance = 1;
			target->follow_distance = 2;
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
		case to_client::game::character_follows::type:
		{
			to_client::game::character_follows packet{ stream };
			auto follower = world.objects.character(packet.follower_id);
			if (follower) {
				follower->follow_object_id = packet.target_id;
			}
			break;
		}
		default:
			break;
		}
	});

	window().set_clear_color({ 160.0f / 255.0f, 230.0f / 255.0f, 1.0f });
}

game_state::~game_state() {
	close_dialogue();
	mouse().press.ignore(mouse_press_id);
	mouse().scroll.ignore(mouse_scroll_id);
	keyboard().press.ignore(keyboard_press_id);
	no::socket_event(server()).packet.ignore(receive_packet_id);
}

void game_state::update() {
	no::synchronize_socket(server());
	renderer.camera.size = window().size().to<float>();
	ui.camera.transform.scale = window().size().to<float>();
	if (world.my_player_id == -1) {
		return;
	}
	follower.update(renderer.camera, world.my_player().object.transform);
	dragger.update(renderer.camera);
	rotater.update(renderer.camera, keyboard());
	world.update();
	renderer.camera.update();
	ui.update();
	chat.update();
	if (dialogue) {
		if (dialogue->is_open()) {
			dialogue->update();
		} else {
			close_dialogue();
		}
	}
	// shift terrain to center player
	no::vector2i player_tile = world.my_player().object.tile();
	no::vector2i world_offset = world.terrain.offset();
	no::vector2i world_size = world.terrain.size();
	no::vector2i goal_offset = player_tile - world_size / 2;
	goal_offset.x = std::max(0, goal_offset.x);
	goal_offset.y = std::max(0, goal_offset.y);
	while (world_offset.x > goal_offset.x) {
		world_offset.x--;
		world.terrain.shift_left();
	}
	while (world_offset.x < goal_offset.x) {
		world_offset.x++;
		world.terrain.shift_right();
	}
	while (world_offset.y > goal_offset.y) {
		world_offset.y--;
		world.terrain.shift_up();
	}
	while (world_offset.y < goal_offset.y) {
		world_offset.y++;
		world.terrain.shift_down();
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
	renderer.draw();
	if (world.my_player_id != -1) {
		if (hovered_pixel.x != -1 && !ui.is_context_open()) {
			renderer.draw_tile_highlights({ hovered_pixel.xy }, { 0.3f, 0.4f, 0.8f, 0.4f });
			if (keyboard().is_key_down(no::key::num_8)) {
				path_found = world.path_between(world.my_player().object.tile(), hovered_pixel.xy);
			}
			renderer.draw_tile_highlights(path_found, { 1.0f, 1.0f, 0.2f, 0.8f });
		}
	}
	ui.draw();
	chat.draw();
	if (dialogue) {
		dialogue->draw();
	}
}

const no::font& game_state::font() const {
	return ui_font;
}

no::vector2i game_state::hovered_tile() const {
	return hovered_pixel.xy;
}

const no::perspective_camera& game_state::world_camera() const {
	return renderer.camera;
}

void game_state::start_dialogue(int target_id) {
	close_dialogue();
	to_server::game::start_dialogue packet;
	packet.target_instance_id = target_id;
	no::send_packet(server(), packet);
	dialogue = new dialogue_view(*this, ui.camera, target_id);
	dialogue->events.choose.listen([this](const dialogue_view::choose_event& event) {
		to_server::game::continue_dialogue packet;
		packet.choice = event.choice;
		no::send_packet(server(), packet);
	});
}

void game_state::close_dialogue() {
	delete dialogue;
	dialogue = nullptr;
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
