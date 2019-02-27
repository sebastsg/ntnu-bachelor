#include "game.hpp"
#include "lobby.hpp"
#include "updater.hpp"
#include "commands.hpp"

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
#if _DEBUG
	no::create_state<lobby_state>("Einheri", 800, 600, 4, true);
#else
	no::create_state<updater_state>("Einheri", 800, 600, 4, true);
#endif
}
