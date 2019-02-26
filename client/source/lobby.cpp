#include "lobby.hpp"
#include "assets.hpp"
#include "input.hpp"
#include "window.hpp"

lobby_state::lobby_state() : font(no::asset_path("fonts/leo.ttf"), 16) {
	receive_packet_id = server().events.receive_packet.listen([this](const no::io_socket::receive_packet_message& event) {
		no::io_stream stream = { event.packet.data(), event.packet.size(), no::io_stream::construct_by::shallow_copy };
		int16_t type = stream.read<int16_t>();
		switch (type) {
		case to_client::lobby::login_status::type:
		{
			to_client::lobby::login_status packet{ stream };
			if (packet.status == 1) {
				//player_name = packet.name;
				to_server::lobby::connect_to_world connect_packet;
				connect_packet.world = 0;
				server().send(no::packet_stream(connect_packet));
			}
			break;
		}
		}
	});
	window().set_clear_color(0.3f);
}

lobby_state::~lobby_state() {
	mouse().press.ignore(mouse_press_id);
	keyboard().press.ignore(keyboard_press_id);
	server().events.receive_packet.ignore(receive_packet_id);
}

void lobby_state::update() {

}

void lobby_state::draw() {

}
