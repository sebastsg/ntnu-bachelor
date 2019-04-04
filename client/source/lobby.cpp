#include "lobby.hpp"
#include "assets.hpp"
#include "input.hpp"
#include "window.hpp"
#include "game.hpp"

lobby_state::lobby_state() : 
	font(no::asset_path("fonts/leo.ttf"), 16),
	login_label(*this, camera),
	status_label(*this, camera),
	login_button(*this, camera),
	username(*this, camera, font), 
	password(*this, camera, font),
	remember_username(*this, camera) {
	camera.zoom = 2.0f;
	shader = no::create_shader(no::asset_path("shaders/sprite"));
	color = no::get_shader_variable("uni_Color");
	receive_packet_id = no::socket_event(server()).packet.listen([this](const no::io_stream& packet) {
		no::io_stream stream{ packet.data(), packet.size(), no::io_stream::construct_by::shallow_copy };
		int16_t type = stream.read<int16_t>();
		switch (type) {
		case to_client::lobby::login_status::type:
		{
			to_client::lobby::login_status packet{ stream };
			if (packet.status == 0) {
				status_label.render(font, "Invalid username or password.");
			} else if (packet.status == 1) {
				player_details.name = packet.name;
				to_server::lobby::connect_to_world connect_packet;
				connect_packet.world = 0;
				no::send_packet(server(), connect_packet);
				change_state<game_state>();
			} else if (packet.status == 2) {
				status_label.render(font, "This player is already logged in.");
			}
			break;
		}
		}
	});
	window().set_clear_color(0.3f);
	login_label.render(font, "Enter your username and password");
	login_button.label.render(font, "Login");
	no::surface button_surface(6, 1, no::pixel_format::rgba);
	button_surface.set(0, 0xFFFFFFFF);
	button_surface.set(1, 0xFFFF7777);
	button_surface.set(2, 0xFFEE5555);
	button_surface.set(3, 0xFF44FFFF);
	button_surface.set(4, 0xFF447777);
	button_surface.set(5, 0xFFAA4455);
	button_texture = no::create_texture(button_surface);
	login_button.events.click.listen([this] {
		save_config();
		to_server::lobby::login_attempt packet;
		packet.name = username.value();
		packet.password = password.value();
		no::send_packet(server(), packet);
	});
	login_button.color = color;
	login_button.set_tex_coords(0.0f, 0.5f);
	no::surface input_surface = { 3, 1, no::pixel_format::rgba };
	input_surface.set(0, 0xFF999999);
	input_surface.set(1, 0xFF666666);
	input_surface.set(2, 0xFF555555);
	input_background = no::create_texture(input_surface);
	password.censor = true;

	remember_username.label.render(font, "Remember username");
	remember_username.set_tex_coords(0.0f, 0.5f);
	remember_username.label_alignment = no::align_type::left;
	remember_username.label_padding.x = 40.0f;
	remember_username.color = color;
	remember_username.events.click.listen([this] {
		remember_username_check = !remember_username_check;
		if (remember_username_check) {
			remember_username.set_tex_coords(0.5f, 0.5f);
		} else {
			remember_username.set_tex_coords(0.0f, 0.5f);
		}
	});
	load_config();

	keyboard_press_id = keyboard().press.listen([this](const no::keyboard::press_message& event) {
		if (event.key == no::key::num_5) {
			username.set_value("test@test.no");
		} else if (event.key == no::key::num_6) {
			username.set_value("sebastian@sgundersen.com");
		}
	});
}

lobby_state::~lobby_state() {
	mouse().press.ignore(mouse_press_id);
	keyboard().press.ignore(keyboard_press_id);
	no::socket_event(server()).packet.ignore(receive_packet_id);
	no::delete_texture(button_texture);
	no::delete_texture(input_background);
	no::delete_shader(shader);
}

void lobby_state::update() {
	no::synchronize_socket(server());
	camera.transform.scale = window().size().to<float>();
	login_label.transform.position = 128.0f;
	status_label.transform.position = { 128.0f, 320.0f };
	username.transform.position = { 128.0f, 160.0f };
	username.transform.scale = { 320.0f, 32.0f };
	password.transform.position = { 128.0f, 208.0f };
	password.transform.scale = username.transform.scale;
	username.update();
	password.update();
	login_button.transform.position = { 128.0f, 264.0f };
	login_button.transform.scale = { 160.0f, 32.0f };
	login_button.update();
	remember_username.transform.position = { 480.0f, 160.0f };
	remember_username.transform.scale = { 32.0f, 32.0f };
	remember_username.update();
}

void lobby_state::draw() {
	no::bind_shader(shader);
	no::set_shader_view_projection(camera);
	color.set(no::vector4f{ 1.0f });
	login_label.draw(rectangle);
	status_label.draw(rectangle);

	no::bind_texture(button_texture);
	login_button.draw_button();
	remember_username.draw_button();

	login_button.draw_label();
	remember_username.draw_label();

	color.set(no::vector4f{ 1.0f });
	username.draw(input_background);
	password.draw(input_background);
}

void lobby_state::save_config() {
	no::io_stream stream;
	stream.write<uint8_t>(remember_username_check ? 1 : 0);
	if (remember_username_check) {
		stream.write(username.value());
	}
	no::file::write("config.data", stream);
}

void lobby_state::load_config() {
	no::io_stream stream;
	no::file::read("config.data", stream);
	if (stream.size_left_to_read() == 0) {
		return;
	}
	remember_username_check = (stream.read<uint8_t>() != 0);
	if (remember_username_check) {
		remember_username.set_tex_coords(0.5f, 0.5f);
		username.set_value(stream.read<std::string>());
	}
}
