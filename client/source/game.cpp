#include "game.hpp"
#include "window.hpp"
#include "debug.hpp"
#include "surface.hpp"
#include "io.hpp"
#include "player.hpp"
#include "platform.hpp"
#include "commands.hpp"
#include "packets.hpp"

hud_view::hud_view() : font("arial.ttf", 20) {
	shader = no::create_shader(no::asset_path("shaders/sprite"));
	fps_texture = no::create_texture();
	debug_texture = no::create_texture();
}

void hud_view::draw() const {
	no::bind_shader(shader);
	no::set_shader_view_projection(camera);
	no::get_shader_variable("uni_Color").set({ 1.0f, 1.0f, 1.0f, 1.0f });

	no::transform transform;
	transform.position.xy = 4.0f;
	transform.scale.xy = no::texture_size(fps_texture).to<float>();
	no::bind_texture(fps_texture);
	no::set_shader_model(transform);
	rectangle.bind();
	rectangle.draw();
	transform.position.y = 24.0f;
	transform.scale.xy = no::texture_size(debug_texture).to<float>();
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

}

void game_world::update() {
	for (auto& player : players) {
		player.second.update();
	}
	camera.update();
}

game_state::game_state() : renderer(world), dragger(mouse()) {
	window().set_swap_interval(no::swap_interval::immediate);
	set_synchronization(no::draw_synchronization::always);

	mouse_press_id = mouse().press.listen([this](const no::mouse::press_message& event) {
		if (event.button != no::mouse::button::left) {
			return;
		}
		no::vector2i tile = hovered_pixel.xy;
		if (tile.x != -1) {
			move_to_tile_packet packet;
			packet.timestamp = 0;
			packet.tile_x = tile.x;
			packet.tile_z = tile.y;

			no::io_stream stream;
			no::packetizer::start(stream);
			packet.write(stream);
			no::packetizer::end(stream);
			server.send_async(stream);
		}
	});

	mouse_scroll_id = mouse().scroll.listen([this](const no::mouse::scroll_message& event) {
		float& factor = world.camera.rotation_offset_factor;
		factor -= (float)event.steps;
		if (factor > 12.0f) {
			factor = 12.0f;
		} else if (factor < 4.0f) {
			factor = 4.0f;
		}
	});

	keyboard_press_id = keyboard().press.listen([this](const no::keyboard::press_message& event) {
		
	});

	server.connect("game.einheri.xyz", 7524); // todo: config file

	server.events.receive_packet.listen([this](const no::io_socket::receive_packet_message& event) {
		no::io_stream stream = { event.packet.data(), event.packet.size(), no::io_stream::construct_by::shallow_copy };
		int16_t type = stream.read<int16_t>();
		switch (type) {
		case move_to_tile_packet::type:
		{
			move_to_tile_packet packet;
			packet.read(stream);
			world.player(packet.player_id).start_movement_to(packet.tile_x, packet.tile_z);
			break;
		}
		case session_packet::type:
		{
			session_packet packet;
			packet.read(stream);
			auto& player = world.add_player(packet.player_id);
			if (packet.is_me) {
				world.my_player_id = packet.player_id;
			}
			world.player(packet.player_id).transform.position.x = (float)packet.tile_x;
			world.player(packet.player_id).transform.position.z = (float)packet.tile_z;
			break;
		}
		default:
			break;
		}
	});
}

game_state::~game_state() {
	mouse().press.ignore(mouse_press_id);
	mouse().press.ignore(mouse_scroll_id);
	keyboard().press.ignore(keyboard_press_id);
}

void game_state::update() {
	world.camera.size = window().size().to<float>();
	hud.camera.transform.scale.xy = window().size().to<float>();
	server.synchronise();
	if (world.my_player_id == -1) {
		return;
	}
	follower.update(world.camera, world.my_player().transform);
	dragger.update(world.camera);
	rotater.update(world.camera, keyboard());
	hud.set_fps(frame_counter().current_fps());
	world.update();
	hud.set_debug(STRING("Tile: " << world.my_player().tile()));

	renderer.highlight_tile = hovered_pixel.xy;
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
	hud.draw();
}

void configure() {
#if _DEBUG
	no::set_asset_directory("../../../assets");
#else
	no::set_asset_directory("assets");
#endif
}

void start() {
	bool no_window = process_command_line();
	if (no_window) {
		return;
	}
	no::create_state<game_state>("Nordgard", 800, 600, 4);
}
