#include "commands.hpp"
#include "platform.hpp"

bool process_command_line() {
	bool no_window = false;
	auto args = no::platform::command_line_arguments();
	size_t i = 0;
	auto args_left = [&](size_t left) {
		return args.size() > left + i;
	};
	for (; i < args.size(); i++) {
		if (args[i] == "--no-window") {
			no_window = true;
		}
	}
	return no_window;
}
