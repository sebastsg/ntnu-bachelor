#include "client.hpp"
#include "window.hpp"
#include "platform.hpp"
#include "network.hpp"

static int server_socket = -1;
client_state::player_details_ client_state::player_details;

client_state::client_state() {
	if (server_socket == -1) {
		server_socket = no::open_socket("game.einheri.xyz", 7524); // todo: config file
	}
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

int client_state::server() {
	return server_socket;
}
