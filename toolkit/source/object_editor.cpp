#include "object_editor.hpp"
#include "window.hpp"
#include "assets.hpp"
#include "platform.hpp"

#include "imgui/imgui.h"
#include "imgui/imgui_platform.h"

object_editor::object_editor() {
	no::imgui::create(window());
	objects.load(no::asset_path("objects.data"));
}

object_editor::~object_editor() {
	no::imgui::destroy();
}

void object_editor::object_type_combo(game_object_type& target, const std::string& ui_id) {
	if (ImGui::BeginCombo(CSTRING("Object type##ObjectType" << ui_id), CSTRING(new_object.type))) {
		for (int type_value = 0; type_value < (int)game_object_type::total_types; type_value++) {
			game_object_type type = (game_object_type)type_value;
			if (ImGui::Selectable(CSTRING(type << "##ObjectType" << ui_id << type_value))) {
				target = type;
			}
		}
		ImGui::End();
	}
}

void object_editor::ui_create_object() {
	if (ImGui::CollapsingHeader("Create new object")) {
		ImGui::InputText("Name##NewObjectName", new_object.name, 100);
		object_type_combo(new_object.type, "New");
		if (ImGui::Button("Load model##NewObjectModel")) {
			std::string browsed_path = no::platform::open_file_browse_window();
			new_object.model.load<no::animated_mesh_vertex>(browsed_path);
		}
		if (ImGui::Button("Save##SaveNewObject")) {
			game_object_definition object;
			object.id = objects.count();
			object.type = new_object.type;
			object.name = new_object.name;
			objects.add(object);
			new_object = {};
		}
	}
}

void object_editor::ui_select_object() {
	ImGui::Text("Select object:");
	ImGui::SameLine();
	if (ImGui::BeginCombo("##SelectEditObject", objects.get(current_object).name.c_str())) {
		for (int id = 0; id < objects.count(); id++) {
			if (ImGui::Selectable(objects.get(id).name.c_str())) {
				current_object = id;
			}
		}
		ImGui::EndCombo();
	}
	auto& selected = objects.get(current_object);
	if (selected.id == -1) {
		return;
	}
	ImGui::InputText("Name##EditObjectName", selected.name.data(), selected.name.capacity());
	object_type_combo(selected.type, "Edit");
}

void object_editor::update() {
	no::imgui::start_frame();
	menu_bar_state::update();
	ImGuiWindowFlags imgui_flags
		= ImGuiWindowFlags_NoMove
		| ImGuiWindowFlags_NoResize
		| ImGuiWindowFlags_NoCollapse
		| ImGuiWindowFlags_NoTitleBar;
	ImGui::SetNextWindowPos({ 0.0f, 20.0f }, ImGuiSetCond_Once);
	ImGui::SetNextWindowSize({ 320.0f, (float)window().height() - 20.0f }, ImGuiSetCond_Always);
	ImGui::Begin("##ObjectEditor", nullptr, imgui_flags);

	ui_create_object();
	ImGui::Separator();
	ui_select_object();

	ImGui::Separator();

	if (ImGui::Button("Save")) {
		objects.save(no::asset_path("objects.data"));
	}

	ImGui::End();
	no::imgui::end_frame();
}

void object_editor::draw() {
	no::imgui::draw();
}
