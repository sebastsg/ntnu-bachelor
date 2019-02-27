#include "lobby.hpp"
#include "assets.hpp"
#include "input.hpp"
#include "window.hpp"
#include "game.hpp"

lobby_state::lobby_state() : 
	font(no::asset_path("fonts/leo.ttf"), 16),
	login_label(*this, camera), 
	login_button(*this, camera),
	username(*this, camera, font), 
	password(*this, camera, font) {
	camera.zoom = 2.0f;
	shader = no::create_shader(no::asset_path("shaders/sprite"));
	color = no::get_shader_variable("uni_Color");
	receive_packet_id = server().events.receive_packet.listen([this](const no::io_socket::receive_packet_message& event) {
		no::io_stream stream = { event.packet.data(), event.packet.size(), no::io_stream::construct_by::shallow_copy };
		int16_t type = stream.read<int16_t>();
		switch (type) {
		case to_client::lobby::login_status::type:
		{
			to_client::lobby::login_status packet{ stream };
			if (packet.status == 1) {
				player_details.name = packet.name;
				to_server::lobby::connect_to_world connect_packet;
				connect_packet.world = 0;
				server().send(no::packet_stream(connect_packet));
				change_state<game_state>();
			}
			break;
		}
		}
	});
	window().set_clear_color(0.3f);
	login_label.render(font, "Enter your username and password");
	login_button.label.render(font, "Login");
	no::surface surface(3, 1, no::pixel_format::rgba);
	surface.set(0, 0xFFFFFFFF);
	surface.set(1, 0xFFFF7777);
	surface.set(2, 0xFFEE5555);
	button_texture = no::create_texture(surface);
	login_button.events.click.listen([this] {
		to_server::lobby::login_attempt packet;
		packet.name = username.value();
		packet.password = password.value();
		server().send_async(no::packet_stream(packet));
	});
	login_button.color = color;
	surface.set(0, 0xFF999999);
	surface.set(1, 0xFF666666);
	surface.set(2, 0xFF555555);
	input_background = no::create_texture(surface);
	password.censor = true;
}

lobby_state::~lobby_state() {
	mouse().press.ignore(mouse_press_id);
	keyboard().press.ignore(keyboard_press_id);
	server().events.receive_packet.ignore(receive_packet_id);
	no::delete_texture(button_texture);
	no::delete_texture(input_background);
	no::delete_shader(shader);
}

void lobby_state::update() {
	server().synchronise();
	camera.transform.scale = window().size().to<float>();
	login_label.transform.position = 128.0f;
	username.transform.position = { 128.0f, 160.0f };
	username.transform.scale = { 320.0f, 32.0f };
	password.transform.position = { 128.0f, 208.0f };
	password.transform.scale = username.transform.scale;
	username.update();
	password.update();
	login_button.transform.position = { 128.0f, 264.0f };
	login_button.transform.scale = { 160.0f, 32.0f };
	login_button.update();
}

void lobby_state::draw() {
	no::bind_shader(shader);
	no::set_shader_view_projection(camera);
	color.set(no::vector4f{ 1.0f });
	login_label.draw(rectangle);
	login_button.draw(button_texture);
	color.set(no::vector4f{ 1.0f });
	username.draw(input_background);
	password.draw(input_background);
}
