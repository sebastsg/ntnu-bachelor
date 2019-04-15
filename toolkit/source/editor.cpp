#include "editor.hpp"
#include "commands.hpp"

#include "window.hpp"
#include "debug.hpp"
#include "surface.hpp"
#include "io.hpp"
#include "character.hpp"

#include "imgui/imgui.h"
#include "imgui/imgui_platform.h"

#include <filesystem>

editor_world::editor_world() {
	name = "main";
	objects.load();
	terrain.load(1);
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
	ImGui::RadioButton("Grass", &current_type, world_tile::grass);
	ImGui::RadioButton("Dirt", &current_type, world_tile::dirt);
}

void tiling_tool::draw() {

}

tile_flag_tool::tile_flag_tool(world_editor_state& editor) : world_editor_tool(editor) {

}

void tile_flag_tool::update() {
	if (editor.keyboard().is_key_down(no::key::space)) {
		if (editor.mouse().is_button_down(no::mouse::button::left)) {
			apply();
		}
	}
}

void tile_flag_tool::update_imgui() {
	int before = flag;
	ImGui::RadioButton("Solid", &flag, world_tile::solid_flag);
	ImGui::RadioButton("Water", &flag, world_tile::water_flag);
	if (before != flag) {
		refresh();
	}
	ImGui::Checkbox("Enable flag", &value);
	if (flag == world_tile::water_flag) {
		ImGui::InputFloat("Water height", &water_elevation);
	}
}

void tile_flag_tool::draw() {
	editor.renderer.draw_tile_highlights(tiles_with_flag, { 1.0f, 0.0f, 0.0f, 0.5f });
}

void tile_flag_tool::refresh() {
	auto& terrain = editor.world.terrain;
	no::vector2i offset = terrain.offset();
	no::vector2i size = terrain.size();
	tiles_with_flag.clear();
	for (int x = offset.x; x < size.x; x++) {
		for (int y = offset.y; y < size.y; y++) {
			if (flag == world_tile::solid_flag && editor.world.terrain.tile_at({ x, y }).is_solid()) {
				tiles_with_flag.emplace_back(x, y);
			}
			if (flag == world_tile::water_flag && editor.world.terrain.tile_at({ x, y }).is_water()) {
				tiles_with_flag.emplace_back(x, y);
			}
		}
	}
}

void tile_flag_tool::apply() {
	for (auto& tile : editor.brush_tiles) {
		bool before = editor.world.terrain.tile_at(tile).flag(flag);
		if (value) {
			editor.world.terrain.tile_at(tile).water_height = water_elevation;
		}
		if (value != before) {
			editor.world.terrain.set_tile_flag(tile, flag, value);
			if (before) {
				erase(tile);
			} else {
				tiles_with_flag.push_back(tile);
			}
		}
	}
}

void tile_flag_tool::erase(no::vector2i tile) {
	for (int i = 0; i < (int)tiles_with_flag.size(); i++) {
		if (tiles_with_flag[i] == tile) {
			tiles_with_flag.erase(tiles_with_flag.begin() + i);
			break;
		}
	}
}

object_tool::object_tool(world_editor_state& editor) : world_editor_tool(editor) {
	ui_texture = no::create_texture({ no::asset_path("sprites/ui.png") });
}

object_tool::~object_tool() {
	disable();
	//editor.renderer.remove(object);
	no::delete_texture(ui_texture);
}

void object_tool::enable() {
	mouse_press_id = editor.mouse().press.listen([this](const no::mouse::press_message& event) {
		if (event.button != no::mouse::button::left || editor.is_mouse_over_ui()) {
			return;
		}
		if (!editor.keyboard().is_key_down(no::key::space)) {
			return;
		}
		int object_id = editor.world.objects.add(object_definition_id);
		auto& object = editor.world.objects.object(object_id);
		object.transform.position = editor.world_position_for_tile(editor.selected_tile);
		object.transform.position.x += 0.5f;
		object.transform.position.z += 0.5f;
	});
}

void object_tool::disable() {
	editor.mouse().press.ignore(mouse_press_id);
	mouse_press_id = -1;
}

void object_tool::update() {
	if (editor.is_mouse_over_ui() || editor.world.terrain.is_out_of_bounds(editor.hovered_tile)) {
		return;
	}
	object.transform.position = editor.world_position_for_tile(editor.hovered_tile);
	object.transform.position.x += 0.5f;
	object.transform.position.z += 0.5f;
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
		//editor.renderer.remove(object);
		object = {};
		object.definition_id = object_definition_id;
		//editor.renderer.add(object);
	}

	if (ImGui::IsMouseClicked(1) && !editor.is_mouse_over_ui()) { // right click in world
		ImGui::OpenPopup("SelectObjectInWorld");
		context_menu_tile = editor.hovered_tile;
	}
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
	if (ImGui::BeginPopup("SelectObjectInWorld")) {
		editor.world.objects.for_each([this](game_object* object) {
			if (object->tile() != context_menu_tile) {
				return;
			}
			if (ImGui::MenuItem(CSTRING(object->definition().name << " (" << object->instance_id << ")"))) {
				selected_object_instance_id = object->instance_id;
			}
		});
		ImGui::EndPopup();
	}
	ImGui::PopStyleVar();

	if (selected_object_instance_id == -1) {
		return;
	}
	auto& object = editor.world.objects.object(selected_object_instance_id);
	ImGui::Separator();
	ImGui::PushID(CSTRING("SelectedObject"));
	ImGui::Text(CSTRING("Selected object: " << object.definition().name << " (" << object.instance_id << ")"));
	ImGui::Text("Transform");
	ImGui::InputFloat3("Position", &object.transform.position.x);
	ImGui::InputFloat3("Scale", &object.transform.scale.x);
	ImGui::InputFloat3("Rotation", &object.transform.rotation.x);

	if (object.definition().type == game_object_type::character) {
		auto character = editor.world.objects.character(object.instance_id);
		auto& health = character->stat(stat_type::health);
		int health_level = health.real();
		ImGui::InputInt("Health", &health_level);
		if (health_level != health.real()) {
			health.set_experience(health.experience_for_level(health_level));
		}
		ImGui::Separator();
		ImGui::Checkbox("Walking around", &character->walking_around);
		character->walking_around_center = object.tile();
		ImGui::Separator();
		ImGui::Text("Equipment");
		auto& equipment = character->equipment;
		bool dirty = false;
		for (int slot = 1; slot < (int)equipment_slot::total_slots; slot++) {
			auto& item = equipment.items[(int)slot];
			ImGui::Text(CSTRING((equipment_slot)slot << ":"));
			ImGui::SameLine();
			no::vector2f uv;
			if (item.definition_id != -1) {
				uv = item.definition().uv;
			}
			ImGui::ImageButton((ImTextureID)ui_texture, { 32.0f }, item_uv1(uv, ui_texture), item_uv2(uv, ui_texture), 1);
			item_popup_context(CSTRING("##Equip" << &item), &item, ui_texture, dirty);
		}
	}

	if (ImGui::Button("Delete")) {
		editor.world.objects.remove(selected_object_instance_id);
		selected_object_instance_id = -1;
	}
	ImGui::PopID();
}

void object_tool::draw() {
	
}

world_editor_state::world_editor_state() : renderer(world), dragger(mouse()), elevate(*this), tiling(*this), object(*this), tile_flag(*this) {
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
	tool().enable();
	tile_flag.refresh();
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
	no::vector2i previous_offset = world.terrain.offset();
	world.terrain.shift_to_center_of(world_position_to_tile_index(renderer.camera.transform.position));
	if (previous_offset != world.terrain.offset()) {
		renderer.update_object_visibility();
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
	ImGui::RadioButton("Tile flags", &radio_tool_id, tile_flag_tool_id);
	if (radio_tool_id != current_tool_id) {
		tool().disable();
		current_tool_id = radio_tool_id;
		tool().enable();
	}

	for (int i = 0; i < 9; i += 3) {
		ImGui::Text(CSTRING("[" << world.terrain.chunks[i].index() << "]"));
		ImGui::SameLine();
		ImGui::Text(CSTRING("[" << world.terrain.chunks[i + 1].index() << "]"));
		ImGui::SameLine();
		ImGui::Text(CSTRING("[" << world.terrain.chunks[i + 2].index() << "]"));
	}

	ImGui::Separator();
	tool().update_imgui();
	ImGui::Separator();
	if (ImGui::Button("Save")) {
		world.objects.save();
		world.terrain.save();
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

	renderer.light.position = renderer.camera.transform.position + renderer.camera.offset();
	renderer.draw();
	renderer.draw_tile_highlights(brush_tiles, { 1.0f, 0.0f, 0.0f, 0.7f });
	tool().draw();

	if (show_wireframe) {
		no::set_polygon_render_mode(no::polygon_render_mode::wireframe);
		renderer.draw_terrain();
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
	case tile_flag_tool_id: return tile_flag;
	default: return elevate;
	}
}
