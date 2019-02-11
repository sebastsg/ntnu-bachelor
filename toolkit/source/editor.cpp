#include "editor.hpp"
#include "commands.hpp"

#include "window.hpp"
#include "debug.hpp"
#include "surface.hpp"
#include "io.hpp"

#include "imgui/imgui.h"
#include "imgui/imgui_platform.h"

#include <filesystem>

editor_world::editor_world() {
	load(no::asset_path("worlds/main.ew"));
}

void editor_world::update() {
	
}

world_editor_tool::world_editor_tool(world_editor_state& editor) : editor(editor) {

}

elevate_tool::elevate_tool(world_editor_state& editor) : world_editor_tool(editor) {

}

void elevate_tool::update() {
	if (editor.keyboard().is_key_down(no::key::space)) {
		if (editor.mouse().is_button_down(no::mouse::button::left)) {
			for (auto& tile : editor.brush_tiles) {
				if (limit_elevation) {
					editor.world.terrain.set_elevation_at(tile, elevation_limit);
				} else {
					editor.world.terrain.elevate_tile(tile, elevation_rate);
				}
			}
		} else if (editor.mouse().is_button_down(no::mouse::button::right)) {
			for (auto& tile : editor.brush_tiles) {
				if (limit_elevation) {
					editor.world.terrain.set_elevation_at(tile, 0.0f);
				} else {
					editor.world.terrain.elevate_tile(tile, -elevation_rate);
				}
			}
		}
	}
}

void elevate_tool::update_imgui() {
	ImGui::InputFloat("Elevation rate", &elevation_rate, 0.01f, 0.01f, 2, limit_elevation ? ImGuiInputTextFlags_ReadOnly : 0);
	elevation_rate = std::min(std::max(elevation_rate, 0.01f), 0.5f);
	ImGui::Checkbox("Limit##LimitElevationCheck", &limit_elevation);
	ImGui::SameLine();
	ImGui::InputFloat("##LimitElevationValue", &elevation_limit, 0.1f, 1.0f, 2, limit_elevation ? 0 : ImGuiInputTextFlags_ReadOnly);
	elevation_limit = std::min(std::max(elevation_limit, 0.0f), 100.0f);
}

void elevate_tool::draw() {

}

tiling_tool::tiling_tool(world_editor_state& editor) : world_editor_tool(editor) {

}

void tiling_tool::update() {
	if (editor.keyboard().is_key_down(no::key::space)) {
		if (editor.mouse().is_button_down(no::mouse::button::left)) {
			for (auto& tile : editor.brush_tiles) {
				editor.world.terrain.set_tile_type(tile, current_type);
			}
		}
	}
}

void tiling_tool::update_imgui() {
	ImGui::RadioButton("Grass", &current_type, world_autotiler::grass);
	ImGui::RadioButton("Dirt", &current_type, world_autotiler::dirt);
	ImGui::RadioButton("Water", &current_type, world_autotiler::water);
}

void tiling_tool::draw() {

}

object_tool::object_tool(world_editor_state& editor) : world_editor_tool(editor) {

}

object_tool::~object_tool() {
	disable();
	editor.renderer.remove(object);
	delete object;
}

void object_tool::enable() {
	mouse_press_id = editor.mouse().press.listen([this](const no::mouse::press_message& event) {
		if (event.button != no::mouse::button::left || editor.is_mouse_over_ui()) {
			return;
		}
		auto object = editor.world.objects.add(object_definition_id);
		if (!object) {
			WARNING("Failed to add object: " << object_definition_id);
			return;
		}
		object->change_id(editor.world.objects.next_static_id());
		object->transform.position = editor.world_position_for_tile(editor.selected_tile);
		object->transform.position.x += 0.5f;
		object->transform.position.z += 0.5f;
	});
}

void object_tool::disable() {
	editor.mouse().press.ignore(mouse_press_id);
	mouse_press_id = -1;
}

void object_tool::update() {
	if (!object || editor.is_mouse_over_ui() || editor.world.terrain.is_out_of_bounds(editor.hovered_tile)) {
		return;
	}
	object->transform.position = editor.world_position_for_tile(editor.hovered_tile);
	object->transform.position.x += 0.5f;
	object->transform.position.z += 0.5f;
}

void object_tool::update_imgui() {
	int object_id = object_definition_id;
	ImGui::ListBox("Objects", &object_id, [](void* data, int i, const char** out) -> bool {
		if (i < 0 || i >= object_definitions().count()) {
			return false;
		}
		*out = object_definitions().get(i).name.c_str();
		return true;
	}, nullptr, object_definitions().count(), 20);
	if (object_id != object_definition_id) {
		object_definition_id = object_id;
		editor.renderer.remove(object);
		delete object;
		object = object_definitions().construct(object_definition_id);
		editor.renderer.add(object);
	}
}

void object_tool::draw() {
	
}

world_editor_state::world_editor_state() : renderer(world), dragger(mouse()), elevate(*this), tiling(*this), object(*this) {
	window().set_swap_interval(no::swap_interval::immediate);
	set_synchronization(no::draw_synchronization::if_updated);
	no::imgui::create(window());
	mouse_press_id = mouse().press.listen([this](const no::mouse::press_message& event) {
		if (event.button != no::mouse::button::left || is_mouse_over_ui()) {
			return;
		}
		selected_tile = hovered_tile;
		is_selected = true;
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
	window().set_clear_color({ 160.0f / 255.0f, 230.0f / 255.0f, 1.0f });
	renderer.fog.start = 100.0f;
	tool().enable();
}

world_editor_state::~world_editor_state() {
	mouse().press.ignore(mouse_press_id);
	mouse().release.ignore(mouse_release_id);
	mouse().scroll.ignore(mouse_scroll_id);
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
	tool().update();
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

	int radio_tool_id = current_tool_id;
	ImGui::RadioButton("Elevate", &radio_tool_id, elevate_tool_id);
	ImGui::RadioButton("Tile", &radio_tool_id, tiling_tool_id);
	ImGui::RadioButton("Object", &radio_tool_id, object_tool_id);
	if (radio_tool_id != current_tool_id) {
		tool().disable();
		current_tool_id = radio_tool_id;
		tool().enable();
	}

	ImGui::Separator();
	tool().update_imgui();
	ImGui::Separator();
	if (ImGui::Button("Save")) {
		world.save(no::asset_path("worlds/main.ew"));
	}
	ImGui::Separator();
	if (ImGui::Button("/\\")) {
		world.terrain.shift_up();
	}
	if (ImGui::Button("<-")) {
		world.terrain.shift_left();
	}
	ImGui::SameLine();
	if (ImGui::Button("->")) {
		world.terrain.shift_right();
	}
	if (ImGui::Button("\\/")) {
		world.terrain.shift_down();
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
	renderer.draw_tile_highlight(hovered_tile);
	for (auto& tile : brush_tiles) {
		renderer.draw_tile_highlight(tile);
	}
	tool().draw();

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

world_editor_tool& world_editor_state::tool() {
	switch (current_tool_id) {
	case elevate_tool_id: return elevate;
	case tiling_tool_id: return tiling;
	case object_tool_id: return object;
	default: return elevate;
	}
}
