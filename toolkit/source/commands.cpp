#include "commands.hpp"

#include "draw.hpp"
#include "assets.hpp"
#include "platform.hpp"
#include "io.hpp"
#include "vertex.hpp"

#include "render.hpp"

#include <filesystem>

static void convert_model_command(const std::string& source_model_name) {
	auto directory = no::asset_path("models/source/decorations");
	auto files = no::entries_in_directory(directory, no::entry_inclusion::only_files);
	no::model_conversion_options decoration_options;
	decoration_options.exporter = [](const std::string& path, const no::model_data<no::animated_mesh_vertex>& model) {
		no::export_model<no::static_textured_vertex>(path, model.to<no::static_textured_vertex>([](const no::animated_mesh_vertex& vertex) {
			return no::static_textured_vertex{ vertex.position, vertex.tex_coords };
		}));
	};
	for (auto& file : files) {
		auto extension = no::file_extension_in_path(file);
		if (extension != ".dae" && extension != ".obj") {
			WARNING("Cannot import " << file << ". Only COLLADA and OBJ is supported.");
			continue;
		}
		std::string name = std::filesystem::path(file).stem().string();
		no::convert_model(file, no::asset_path("models/decorations/" + name + ".nom"), decoration_options);
	}

	no::model_conversion_options player_options;
	player_options.exporter = [](const std::string& path, const no::model_data<no::animated_mesh_vertex>& model) {
		no::export_model(path, model);
	};
	no::convert_model(no::asset_path("models/source/player/idle.dae"), no::asset_path("models/source/player/idle.nom"), player_options);
	no::convert_model(no::asset_path("models/source/player/run.dae"), no::asset_path("models/source/player/run.nom"), player_options);
	no::convert_model(no::asset_path("models/source/player/swing.dae"), no::asset_path("models/source/player/swing.nom"), player_options);
	no::convert_model(no::asset_path("models/source/player/shielding.dae"), no::asset_path("models/source/player/shielding.nom"), player_options);
	no::merge_model_animations<no::animated_mesh_vertex>(no::asset_path("models/source/player"), no::asset_path("models/player.nom"));

	no::model_conversion_options equipment_options;
	equipment_options.exporter = [](const std::string& path, const no::model_data<no::animated_mesh_vertex>& model) {
		no::export_model(path, model);
	};
	equipment_options.import.bones = { true, "sword" };
	no::convert_model(no::asset_path("models/source/sword/idle.dae"), no::asset_path("models/source/sword/idle.nom"), equipment_options);
	no::convert_model(no::asset_path("models/source/sword/run.dae"), no::asset_path("models/source/sword/run.nom"), equipment_options);
	no::convert_model(no::asset_path("models/source/sword/swing.dae"), no::asset_path("models/source/sword/swing.nom"), equipment_options);
	no::merge_model_animations<no::animated_mesh_vertex>(no::asset_path("models/source/sword"), no::asset_path("models/sword.nom"));
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
