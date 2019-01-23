#include "editor.hpp"
#include "commands.hpp"
#include "model_manager.hpp"

void configure() {
#if _DEBUG
	no::set_asset_directory("../../../assets");
#else
	no::set_asset_directory("../../../assets");
	//no::set_asset_directory("assets");
#endif
}

void start() {
	bool no_window = process_command_line();
	if (no_window) {
		return;
	}
	no::create_state<model_manager_state>("Toolkit", 800, 600, 4, true);
}
