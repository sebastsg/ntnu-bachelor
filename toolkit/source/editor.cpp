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
		if (event.key == no::key::v) {
			std::vector<float> heights;
			for (int i = 0; i < world.terrain.tile_heights.rows(); i++) {
				heights.push_back(4.0f + (float)i / 32.0f);
			}
			world.terrain.tile_heights.shift_left(heights);
			world.is_dirty = true;
		}
		if (event.key == no::key::h) {
			std::vector<float> heights;
			for (int i = 0; i < world.terrain.tile_heights.rows(); i++) {
				heights.push_back(12.0f + (float)i / 32.0f);
			}
			world.terrain.tile_heights.shift_right(heights);
			world.is_dirty = true;
		}
		if (event.key == no::key::o) {
			std::vector<float> heights;
			for (int i = 0; i < world.terrain.tile_heights.columns(); i++) {
				heights.push_back(4.0f + (float)i / 32.0f);
			}
			world.terrain.tile_heights.shift_up(heights);
			world.is_dirty = true;
		}
		if (event.key == no::key::n) {
			std::vector<float> heights;
			for (int i = 0; i < world.terrain.tile_heights.columns(); i++) {
				heights.push_back(8.0f + (float)i / 32.0f);
			}
			world.terrain.tile_heights.shift_down(heights);
			world.is_dirty = true;
		}
	});
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
			world.terrain.elevate_tile(hovered_tile, 0.5f);
			world.is_dirty = true;
		} else if (mouse().is_button_down(no::mouse::button::right)) {
			world.terrain.elevate_tile(hovered_tile, -0.5f);
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
	ImGui::Text(CSTRING("Position: " << world.terrain.tile_heights.x() << ", " << world.terrain.tile_heights.y()));
	ImGui::Text(CSTRING("Tile: " << hovered_tile));
	ImGui::Text(CSTRING("Pixel: " << hovered_pixel));
	ImGui::Checkbox("Show wireframe", &show_wireframe);
	ImGui::Separator();

	ImGui::End();
	no::imgui::end_frame();
}

void world_editor_state::draw() {
	renderer.draw_for_picking();
	hovered_pixel = no::read_pixel_at({ mouse().x(), window().height() - mouse().y() });
	hovered_pixel.x--;
	hovered_pixel.y--;
	hovered_tile = hovered_pixel.xy + world.terrain.tile_heights.position();
	renderer.highlight_tile = hovered_tile;
	window().clear();
	world.camera.size = window().size().to<float>();
	renderer.draw();
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
	no::create_state<world_editor_state>("Toolkit", 800, 600, 4);
}
