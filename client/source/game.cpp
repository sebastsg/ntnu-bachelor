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

character_object* game_world::my_player() {
	return (character_object*)objects.find(my_player_id);
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
			server().send_async(no::packet_stream(packet));
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
		server().send_async(no::packet_stream(packet));
	});

	receive_packet_id = server().events.receive_packet.listen([this](const no::io_socket::receive_packet_message& event) {
		no::io_stream stream = { event.packet.data(), event.packet.size(), no::io_stream::construct_by::shallow_copy };
		int16_t type = stream.read<int16_t>();
		switch (type) {
		case to_client::game::move_to_tile::type:
		{
			to_client::game::move_to_tile packet{ stream };
			auto player = (character_object*)world.objects.find(packet.player_instance_id);
			if (player) {
				pathfinder pathfind{ world.terrain };
				player->start_path_movement(pathfind.find_path(player->tile(), packet.tile));
			} else {
				WARNING("player not found: " << packet.player_instance_id);
			}
			break;
		}
		case to_client::game::my_player_info::type:
		{
			to_client::game::my_player_info packet{ stream };
			auto player = (character_object*)world.objects.add(packet.player.serialized());
			world.my_player_id = packet.player.id();
			ui.listen(world.my_player());
			variables = packet.variables;
			quests = packet.quests;
			break;
		}
		case to_client::game::other_player_joined::type:
		{
			to_client::game::other_player_joined packet{ stream };
			world.objects.add(packet.player.serialized());
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
			auto attacker = (character_object*)world.objects.find(packet.attacker_id);
			auto target = (character_object*)world.objects.find(packet.target_id);
			attacker->events.attack.emit();
			target->events.defend.emit();
			break;
		}
		case to_client::game::character_equips::type:
		{
			to_client::game::character_equips packet{ stream };
			auto character = (character_object*)world.objects.find(packet.instance_id);
			if (character) {
				character->equip({ packet.item_id, packet.stack });
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
	server().events.receive_packet.ignore(receive_packet_id);
}

void game_state::update() {
	renderer.camera.size = window().size().to<float>();
	ui.camera.transform.scale = window().size().to<float>();
	server().synchronise();
	if (world.my_player_id == -1) {
		return;
	}
	follower.update(renderer.camera, world.my_player()->transform);
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
}

void game_state::draw() {
	if (world.my_player_id == -1) {
		return;
	}
	renderer.draw_for_picking();
	hovered_pixel = no::read_pixel_at({ mouse().x(), window().height() - mouse().y() });
	hovered_pixel.x--;
	hovered_pixel.y--;
	window().clear();
	renderer.draw();
	if (world.my_player_id != -1) {
		if (hovered_pixel.x != -1) {
			renderer.draw_tile_highlights({ hovered_pixel.xy }, { 0.3f, 0.4f, 0.8f, 0.4f });
			if (keyboard().is_key_down(no::key::num_8)) {
				pathfinder p{ world.terrain };
				path_found = p.find_path(world.my_player()->tile(), hovered_pixel.xy);
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
	server().send_async(no::packet_stream(packet));
	dialogue = new dialogue_view(*this, ui.camera, target_id);
	dialogue->events.choose.listen([this](const dialogue_view::choose_event& event) {
		to_server::game::continue_dialogue packet;
		packet.choice = event.choice;
		server().send_async(no::packet_stream(packet));
	});
}

void game_state::close_dialogue() {
	delete dialogue;
	dialogue = nullptr;
}

void game_state::start_combat(int target_id) {
	to_server::game::start_combat packet;
	packet.target_id = target_id;
	server().send_async(no::packet_stream(packet));
}

void game_state::equip_from_inventory(no::vector2i slot) {
	world.my_player()->equip_from_inventory(slot);
	to_server::game::equip_from_inventory packet;
	packet.slot = slot;
	server().send_async(no::packet_stream(packet));
}
