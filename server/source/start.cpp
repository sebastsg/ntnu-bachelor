#include "server.hpp"
#include "assets.hpp"

void configure() {
#if _DEBUG
	no::set_asset_directory("../../../assets");
#else
	no::set_asset_directory("../../../assets");
	//no::set_asset_directory("assets");
#endif
}

void start() {
	no::create_state<server_state>("Server", 800, 600, 4, false);
}