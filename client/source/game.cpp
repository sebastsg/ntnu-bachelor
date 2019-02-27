#include "game.hpp"
#include "window.hpp"
#include "debug.hpp"
#include "surface.hpp"
#include "io.hpp"
#include "platform.hpp"
#include "commands.hpp"
#include "packets.hpp"
#include "network.hpp"

hud_view::hud_view() : font(no::asset_path("fonts/leo.ttf"), 10) {
	shader = no::create_shader(no::asset_path("shaders/sprite"));
	fps_texture = no::create_texture();
	debug_texture = no::create_texture();
}

void hud_view::draw() const {
	no::bind_shader(shader);
	no::set_shader_view_projection(camera);
	no::get_shader_variable("uni_Color").set({ 1.0f, 1.0f, 1.0f, 1.0f });

	no::transform2 transform;
	transform.position = { 300.0f, 4.0f };
	transform.scale = no::texture_size(fps_texture).to<float>();
	no::bind_texture(fps_texture);
	no::set_shader_model(transform);
	rectangle.bind();
	rectangle.draw();
	transform.position.y = 24.0f;
	transform.scale = no::texture_size(debug_texture).to<float>();
	no::bind_texture(debug_texture);
	no::set_shader_model(transform);
	rectangle.draw();
}

void hud_view::set_fps(long long fps) {
	if (this->fps == fps) {
		return;
	}
	no::load_texture(fps_texture, font.render("FPS: " + std::to_string(fps)));
}

void hud_view::set_debug(const std::string& debug) {
	no::load_texture(debug_texture, font.render(debug));
}

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
		if (event.key == no::key::p) {
			//start_dialogue(0);
		}
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
				player->start_movement_to(packet.tile.x, packet.tile.y);
			} else {
				WARNING("player not found: " << packet.player_instance_id);
			}
			break;
		}
		case to_client::game::player_joined::type:
		{
			to_client::game::player_joined packet{ stream };
			auto player = (character_object*)world.objects.add(packet.player.serialized());
			if (packet.is_me) {
				world.my_player_id = packet.player.id();
				ui.listen(world.my_player());
			}
			break;
		}
		case to_client::game::chat_message::type:
		{
			to_client::game::chat_message packet{ stream };
			chat.add(packet.author, packet.message);
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
	hud.camera.transform.scale = window().size().to<float>();
	ui.camera.transform.scale = window().size().to<float>();
	server().synchronise();
	if (world.my_player_id == -1) {
		return;
	}
	follower.update(renderer.camera, world.my_player()->transform);
	dragger.update(renderer.camera);
	rotater.update(renderer.camera, keyboard());
	hud.set_fps(frame_counter().current_fps());
	world.update();
	renderer.camera.update();
	hud.set_debug(STRING("Tile: " << world.my_player()->tile()));
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
		//renderer.draw_tile_highlight(world.my_player()->tile());
	}
	ui.draw();
	chat.draw();
	hud.draw();
	if (dialogue) {
		dialogue->draw();
	}
}

const no::font& game_state::font() const {
	return ui_font;
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
