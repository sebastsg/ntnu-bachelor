#include "editor.hpp"
#include "commands.hpp"
#include "player.hpp"

#include "window.hpp"
#include "debug.hpp"
#include "surface.hpp"
#include "io.hpp"

#include "imgui/imgui.h"
#include "imgui/imgui_platform.h"

#include <filesystem>

editor_world::editor_world() {

}

void editor_world::update() {
	
}

world_editor_state::world_editor_state() : renderer(world), dragger(mouse()) {
	window().set_swap_interval(no::swap_interval::immediate);
	set_synchronization(no::draw_synchronization::if_updated);

	no::imgui::create(window());

	mouse_press_id = mouse().press.listen([this](const no::mouse::press_message& event) {
		if (event.button == no::mouse::button::left) {
			selected_tile = hovered_tile;
			is_selected = true;
			if (tool == 2) {
				decoration_object* obj = world.add_decoration();
				obj->model = "models/decorations/" + object_paths[tool_current_object].first + ".nom";
				obj->transform.scale = 0.01f;
				obj->transform.position = world_position_for_tile(selected_tile);
				renderer.decorations.add(obj);
			}
		}
	});
	mouse_release_id = mouse().release.listen([this](const no::mouse::release_message& event) {
		if (event.button == no::mouse::button::left) {
			is_selected = false;
		}
	});
	mouse_scroll_id = mouse().scroll.listen([this](const no::mouse::scroll_message& event) {
		if (is_selected) {
			world.terrain.elevate_tile(selected_tile, (float)event.steps * 0.5f);
		}
	});
	keyboard_press_id = keyboard().press.listen([this](const no::keyboard::press_message& event) {
		
	});

	no::file::read(no::asset_path("worlds/main.ew"), world_stream);
	world.terrain.read(world_stream);

	auto directory = no::asset_path("models/decorations");
	auto files = no::entries_in_directory(directory, no::entry_inclusion::only_files);
	for (auto& file : files) {
		auto extension = no::file_extension_in_path(file);
		if (extension != ".nom") {
			continue;
		}
		std::string name = std::filesystem::path(file).stem().string();
		object_paths.emplace_back(name, file);
	}
	decorations_texture = no::create_texture(no::surface(no::asset_path("textures/decorations.png")), no::scale_option::nearest_neighbour, true);
	window().set_clear_color({ 160.0f / 255.0f, 230.0f / 255.0f, 1.0f });
	renderer.fog.start = 100.0f;
}

world_editor_state::~world_editor_state() {
	mouse().press.ignore(mouse_press_id);
	mouse().press.ignore(mouse_release_id);
	mouse().press.ignore(mouse_scroll_id);
	keyboard().press.ignore(keyboard_press_id);
	no::imgui::destroy();
}

void world_editor_state::update() {
	renderer.camera.size = window().size().to<float>();
	brush_tiles.clear();
	for (int x = 0; x < brush_size; x++) {
		for (int y = 0; y < brush_size; y++) {
			brush_tiles.emplace_back(x + hovered_tile.x, y + hovered_tile.y);
		}
	}
	world.update();
	update_editor();
	update_imgui();
}

void world_editor_state::update_editor() {
	if (is_mouse_over_ui()) {
		return;
	}
	if (keyboard().is_key_down(no::key::space)) {
		if (mouse().is_button_down(no::mouse::button::left)) {
			for (auto& tile : brush_tiles) {
				if (tool == 0) {
					if (limit_elevation) {
						world.terrain.set_elevation_at(tile, elevation_limit);
					} else {
						world.terrain.elevate_tile(tile, elevation_rate);
					}
				} else if (tool == 1) {
					world.terrain.set_tile_type(tile, current_type);
				}
			}
		} else if (mouse().is_button_down(no::mouse::button::right)) {
			for (auto& tile : brush_tiles) {
				if (tool == 0) {
					if (limit_elevation) {
						world.terrain.set_elevation_at(tile, 0.0f);
					} else {
						world.terrain.elevate_tile(tile, -elevation_rate);
					}
				}
			}
		}
	}
	renderer.camera.update();
	dragger.update(renderer.camera);
	rotater.update(renderer.camera, keyboard());
	mover.update(renderer.camera, keyboard());
}

void world_editor_state::update_imgui() {
	no::imgui::start_frame();
	menu_bar_state::update();
	ImGuiWindowFlags imgui_flags 
		= ImGuiWindowFlags_NoMove 
		| ImGuiWindowFlags_NoResize 
		| ImGuiWindowFlags_NoCollapse 
		| ImGuiWindowFlags_NoTitleBar;
	ImGui::SetNextWindowPos({ 0.0f, 20.0f }, ImGuiSetCond_Once);
	ImGui::SetNextWindowSize({ 320.0f, (float)window().height() - 20.0f }, ImGuiSetCond_Always);
	ImGui::Begin("World", nullptr, imgui_flags);
	ImGui::Text(CSTRING("FPS: " << frame_counter().current_fps()));
	ImGui::Text(CSTRING("World offset: " << world.terrain.offset()));
	ImGui::Text(CSTRING("Tile: " << hovered_tile));

	ImGui::Separator();

	ImGui::Checkbox("Show wireframe", &show_wireframe);

	ImGui::InputInt("Brush size", &brush_size, 1, 1);
	brush_size = std::min(std::max(brush_size, 1), 25);

	ImGui::Separator();

	ImGui::RadioButton("Elevate", &tool, 0);
	ImGui::RadioButton("Tile", &tool, 1);
	ImGui::RadioButton("Object", &tool, 2);

	ImGui::Separator();

	if (tool == 0) {
		ImGui::InputFloat("Elevation rate", &elevation_rate, 0.01f, 0.01f, 2, limit_elevation ? ImGuiInputTextFlags_ReadOnly : 0);
		elevation_rate = std::min(std::max(elevation_rate, 0.01f), 0.5f);
		ImGui::Checkbox("Limit##LimitElevationCheck", &limit_elevation);
		ImGui::SameLine();
		ImGui::InputFloat("##LimitElevationValue", &elevation_limit, 0.1f, 1.0f, 2, limit_elevation ? 0 : ImGuiInputTextFlags_ReadOnly);
		elevation_limit = std::min(std::max(elevation_limit, 0.0f), 100.0f);
	} else if (tool == 1) {
		ImGui::RadioButton("Grass", &current_type, world_autotiler::grass);
		ImGui::RadioButton("Dirt", &current_type, world_autotiler::dirt);
		ImGui::RadioButton("Water", &current_type, world_autotiler::water);
	} else if (tool == 2) {
		int current_obj = tool_current_object;
		ImGui::ListBox("Objects", &tool_current_object, [](void* data, int i, const char** out) -> bool {
			auto& paths = *(std::vector<std::pair<std::string, std::string>>*)data;
			if (i < 0 || i >= (int)paths.size()) {
				return false;
			}
			*out = paths[i].first.c_str();
			return true;
		}, &object_paths, object_paths.size(), 20);
		if (current_obj != tool_current_object) {
			tool_object.load<no::static_textured_vertex>(object_paths[tool_current_object].second);
		}
	}

	ImGui::Separator();

	if (ImGui::Button("Save")) {
		world.terrain.write(world_stream);
		world_stream.set_write_index(world_stream.size());
		no::file::write(no::asset_path("worlds/main.ew"), world_stream);
	}

	ImGui::Separator();

	if (ImGui::Button("/\\")) {
		world.terrain.shift_up(world_stream);
	}
	if (ImGui::Button("<-")) {
		world.terrain.shift_left(world_stream);
	}
	ImGui::SameLine();
	if (ImGui::Button("->")) {
		world.terrain.shift_right(world_stream);
	}
	if (ImGui::Button("\\/")) {
		world.terrain.shift_down(world_stream);
	}

	ImGui::Separator();

	ImGui::End();
	no::imgui::end_frame();
}

void world_editor_state::draw() {
	renderer.draw_for_picking();
	hovered_pixel = no::read_pixel_at({ mouse().x(), window().height() - mouse().y() });
	window().clear();

	hovered_pixel.x--;
	hovered_pixel.y--;
	hovered_tile = hovered_pixel.xy + world.terrain.offset();

	renderer.draw();

	if (tool_object.is_drawable() && !world.terrain.is_out_of_bounds(hovered_tile)) {
		no::transform t;
		t.scale = 0.01f;
		t.position = world_position_for_tile(hovered_tile);
		no::set_shader_model(t);
		no::bind_texture(decorations_texture);
		tool_object.bind();
		tool_object.draw();
	}

	renderer.draw_tile_highlight(hovered_tile);
	for (auto& tile : brush_tiles) {
		renderer.draw_tile_highlight(tile);
	}

	if (show_wireframe) {
		no::set_polygon_render_mode(no::polygon_render_mode::wireframe);
		renderer.draw_for_picking();
		no::set_polygon_render_mode(no::polygon_render_mode::fill);
	}
	no::imgui::draw();
}

bool world_editor_state::is_mouse_over_ui() const {
	return mouse().position().x < 320;
}

no::vector3f world_editor_state::world_position_for_tile(no::vector2i tile) const {
	return { (float)tile.x, world.terrain.elevation_at(tile), (float)tile.y };
}
