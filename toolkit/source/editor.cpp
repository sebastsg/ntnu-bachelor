#include "editor.hpp"
#include "commands.hpp"
#include "player.hpp"

#include "window.hpp"
#include "debug.hpp"
#include "surface.hpp"
#include "io.hpp"

#include "imgui/imgui.h"
#include "imgui/imgui_platform.h"

editor_world::editor_world() {

}

void editor_world::update() {
	camera.update();
}

world_editor_state::world_editor_state() : renderer(world), dragger(mouse()) {
	window().set_swap_interval(no::swap_interval::immediate);
	set_synchronization(no::draw_synchronization::always);

	no::imgui::create(window());

	mouse_press_id = mouse().press.listen([this](const no::mouse::press_message& event) {
		if (event.button == no::mouse::button::left) {
			selected_tile = hovered_tile;
			is_selected = true;
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
			world.is_dirty = true;
		}
	});
	keyboard_press_id = keyboard().press.listen([this](const no::keyboard::press_message& event) {
		
	});

	no::file::read(no::asset_path("worlds/main.ew"), world_stream);
	world.terrain.tile_heights.read(world_stream, 1024, 0.0f);
	world.is_dirty = true;
}

world_editor_state::~world_editor_state() {
	mouse().press.ignore(mouse_press_id);
	mouse().press.ignore(mouse_release_id);
	mouse().press.ignore(mouse_scroll_id);
	keyboard().press.ignore(keyboard_press_id);
	no::imgui::destroy();
}

void world_editor_state::update() {
	if (world.is_dirty) {
		renderer.refresh_terrain();
		world.is_dirty = false;
	}
	world.camera.size = window().size().to<float>();

	brush_tiles.clear();
	int offset_x = hovered_tile.x;
	int offset_y = hovered_tile.y;
	for (int x = 0; x < brush_size; x++) {
		for (int y = 0; y < brush_size; y++) {
			brush_tiles.emplace_back(x + offset_x, y + offset_y);
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
				world.terrain.elevate_tile(tile, 0.1f);
			}
			world.is_dirty = true;
		} else if (mouse().is_button_down(no::mouse::button::right)) {
			for (auto& tile : brush_tiles) {
				world.terrain.elevate_tile(tile, -0.1f);
			}
			world.is_dirty = true;
		}
	}
	dragger.update(world.camera);
	rotater.update(world.camera, keyboard());
	mover.update(world.camera, keyboard());
}

void world_editor_state::update_imgui() {
	no::imgui::start_frame();
	ImGuiWindowFlags imgui_flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar;
	ImGui::SetNextWindowPos({ 0.0f, 0.0f }, ImGuiSetCond_Once);
	ImGui::SetNextWindowSize({ 320.0f, (float)window().height() }, ImGuiSetCond_Always);
	ImGui::Begin("World", nullptr, imgui_flags);
	ImGui::Text(CSTRING("FPS: " << frame_counter().current_fps()));
	ImGui::Text(CSTRING("World offset: " << world.terrain.tile_heights.x() << ", " << world.terrain.tile_heights.y()));
	ImGui::Text(CSTRING("Tile: " << hovered_tile));
	ImGui::Checkbox("Show wireframe", &show_wireframe);
	ImGui::InputInt("Brush size", &brush_size, 1, 1);
	brush_size = std::min(std::max(brush_size, 1), 10);
	ImGui::Separator();

	if (ImGui::Button("Save")) {
		world.terrain.tile_heights.write(world_stream, 1024, 0.0f);
		world_stream.set_write_index(world_stream.size());
		no::file::write(no::asset_path("worlds/main.ew"), world_stream);
	}

	ImGui::Separator();

	if (ImGui::Button("/\\")) {
		world.terrain.tile_heights.shift_up(world_stream, 1024, 0.0f);
		world.is_dirty = true;
	}
	if (ImGui::Button("<-")) {
		world.terrain.tile_heights.shift_left(world_stream, 1024, 0.0f);
		world.is_dirty = true;
	}
	ImGui::SameLine();
	if (ImGui::Button("->")) {
		world.terrain.tile_heights.shift_right(world_stream, 1024, 0.0f);
		world.is_dirty = true;
	}
	if (ImGui::Button("\\/")) {
		world.terrain.tile_heights.shift_down(world_stream, 1024, 0.0f);
		world.is_dirty = true;
	}

	ImGui::End();
	no::imgui::end_frame();
}

void world_editor_state::draw() {
	renderer.draw_for_picking();
	hovered_pixel = no::read_pixel_at({ mouse().x(), window().height() - mouse().y() });
	hovered_pixel.x--;
	hovered_pixel.y--;
	hovered_tile = hovered_pixel.xy + world.terrain.tile_heights.position();
	window().clear();
	world.camera.size = window().size().to<float>();
	renderer.draw();
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
	no::create_state<world_editor_state>("Toolkit", 800, 600, 4, true);
}
