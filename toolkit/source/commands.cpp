#include "commands.hpp"

#include "draw.hpp"
#include "assets.hpp"
#include "platform.hpp"
#include "io.hpp"

static void convert_model_command(const std::string& source_model_name) {
	no::convert_model(no::asset_path("models/source/player/idle.dae"), no::asset_path("models/source/player/idle.nom"));
	no::convert_model(no::asset_path("models/source/player/run.dae"), no::asset_path("models/source/player/run.nom"));
	no::convert_model(no::asset_path("models/source/player/swing.dae"), no::asset_path("models/source/player/swing.nom"));
	no::convert_model(no::asset_path("models/source/player/shielding.dae"), no::asset_path("models/source/player/shielding.nom"));
	no::merge_model_animations(no::asset_path("models/source/player"), no::asset_path("models/player.nom"));

	no::model_importer_options options;
	options.bones = { true, "sword" };
	no::convert_model(no::asset_path("models/source/sword/idle.dae"), no::asset_path("models/source/sword/idle.nom"), options);
	no::convert_model(no::asset_path("models/source/sword/run.dae"), no::asset_path("models/source/sword/run.nom"), options);
	no::convert_model(no::asset_path("models/source/sword/swing.dae"), no::asset_path("models/source/sword/swing.nom"), options);
	no::merge_model_animations(no::asset_path("models/source/sword"), no::asset_path("models/sword.nom"));

	return;

	auto directory = no::asset_path("models/source/" + source_model_name);
	auto files = no::entries_in_directory(directory, no::entry_inclusion::only_files);
	for (auto& file : files) {
		auto extension = no::file_extension_in_path(file);
		if (extension != ".dae" && extension != ".obj") {
			WARNING("Cannot import " << file << ". Only COLLADA and OBJ is supported.");
			continue;
		}

		INFO("\"" << file << "\" has extension \"" << extension << "\"");
	}
}

bool process_command_line() {
	bool no_window = false;
	auto args = no::platform::command_line_arguments();
	//std::vector<std::string> args = { "--no-window", "--convert-model", "player" };
	size_t i = 0;
	auto args_left = [&](size_t left) {
		return args.size() > left + i;
	};
	for (; i < args.size(); i++) {
		if (args[i] == "--no-window") {
			no_window = true;
		} else if (args[i] == "--convert-model") {
			if (args_left(1)) {
				convert_model_command(args[i + 1]);
				i++;
			}
		}
	}
	return no_window;
}
