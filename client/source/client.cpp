#include "client.hpp"
#include "window.hpp"
#include "platform.hpp"

static no::io_socket server_socket;

client_state::player_details_ client_state::player_details;

static void connect_to_server() {
	if (server_socket.id == -1) {
		// todo: config file
		server_socket.connect("game.einheri.xyz", 7524);
	}
}

client_state::client_state() {
	connect_to_server();
	window().set_swap_interval(no::swap_interval::immediate);
	set_synchronization(no::draw_synchronization::always);
	// todo: maybe it's possible to avoid this check, and still use resource icon?
#if PLATFORM_WINDOWS
	constexpr int IDI_ICON1 = 102;
	window().set_icon_from_resource(IDI_ICON1);
#endif
}

std::string client_state::player_name() const {
	return player_details.name;
}

no::io_socket& client_state::server() {
	return server_socket;
}
