#include "client.hpp"
#include "window.hpp"
#include "platform.hpp"

client_state::client_state() {
	// todo: maybe it's possible to avoid this check, and still use resource icon?
#if PLATFORM_WINDOWS
	constexpr int IDI_ICON1 = 102;
	window().set_icon_from_resource(IDI_ICON1);
#endif
}
