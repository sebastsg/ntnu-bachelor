#include "item_editor.hpp"
#include "window.hpp"
#include "assets.hpp"
#include "surface.hpp"

#include "imgui/imgui.h"
#include "imgui/imgui_platform.h"

item_editor::item_editor() {
	no::imgui::create(window());
	items.load(no::asset_path("items.data"));
	ui_texture = no::create_texture(no::surface(no::asset_path("sprites/ui.png")));
}

item_editor::~item_editor() {
	no::delete_texture(ui_texture);
	no::imgui::destroy();
}

void item_editor::item_type_combo(item_type& target, const std::string& ui_id) {
	if (ImGui::BeginCombo(CSTRING("Item type##ItemType" << ui_id), CSTRING(target))) {
		for (int type_value = 0; type_value < (int)item_type::total_types; type_value++) {
			item_type type = (item_type)type_value;
			if (ImGui::Selectable(CSTRING(type << "##ItemType" << ui_id << type_value))) {
				target = type;
			}
		}
		ImGui::End();
	}
}

void item_editor::equipment_slot_combo(equipment_slot& slot, const std::string& ui_id) {
	if (ImGui::BeginCombo(CSTRING("Equipment slot##ItemSlot" << ui_id), CSTRING(slot))) {
		for (int slot_value = 0; slot_value < (int)equipment_slot::total_slots; slot_value++) {
			equipment_slot slot = (equipment_slot)slot_value;
			if (ImGui::Selectable(CSTRING(slot << "##ItemSlot" << ui_id << slot_value))) {
				new_item.slot = slot;
			}
		}
		ImGui::End();
	}
}

void item_editor::ui_create_item() {
	if (ImGui::CollapsingHeader("Create new item")) {
		ImGui::InputText("Name##NewItemName", new_item.name, 100);
		ImGui::InputInt("Max stack##NewItemMaxStack", &new_item.max_stack);
		item_type_combo(new_item.type, "New");
		equipment_slot_combo(new_item.slot, "New");
		if (ImGui::Button("Save##SaveNewItem")) {
			item_definition item;
			item.id = items.count();
			item.name = new_item.name;
			item.max_stack = new_item.max_stack;
			item.type = new_item.type;
			item.slot = new_item.slot;
			items.add(item);
		}
	}
}

void item_editor::ui_select_item() {
	ImGui::Text("Select item:");
	ImGui::SameLine();
	if (ImGui::BeginCombo("##SelectEditItem", items.get(current_item).name.c_str())) {
		for (int id = 0; id < items.count(); id++) {
			if (ImGui::Selectable(items.get(id).name.c_str())) {
				current_item = id;
			}
		}
		ImGui::EndCombo();
	}
	auto& selected = items.get(current_item);
	if (selected.id == -1) {
		return;
	}
	ImGui::InputText("Name##EditItemName", selected.name.data(), selected.name.capacity());
	int max_stack = (int)selected.max_stack;
	ImGui::InputInt("Max stack##EditItemMaxStack", &max_stack);
	max_stack = std::max(1, max_stack);
	selected.max_stack = (long long)max_stack;
	item_type_combo(selected.type, "Edit");
	equipment_slot_combo(selected.slot, "Edit");
	ImGui::InputFloat2("UV##EditItemUV", &selected.uv.x, 0);
	no::vector2f uv_start = selected.uv / no::texture_size(ui_texture).to<float>();
	no::vector2f uv_end = (selected.uv + 32.0f) / no::texture_size(ui_texture).to<float>();
	ImGui::Image((ImTextureID)ui_texture, { 64.0f, 64.0f }, { uv_start }, { uv_end }, { 1.0f }, { 1.0f });
}

void item_editor::update() {
	no::imgui::start_frame();
	menu_bar_state::update();
	ImGuiWindowFlags imgui_flags
		= ImGuiWindowFlags_NoMove
		| ImGuiWindowFlags_NoResize
		| ImGuiWindowFlags_NoCollapse
		| ImGuiWindowFlags_NoTitleBar;
	ImGui::SetNextWindowPos({ 0.0f, 20.0f }, ImGuiSetCond_Once);
	ImGui::SetNextWindowSize({ 320.0f, (float)window().height() - 20.0f }, ImGuiSetCond_Always);
	ImGui::Begin("##ItemEditor", nullptr, imgui_flags);

	ui_create_item();
	ImGui::Separator();
	ui_select_item();

	ImGui::Separator();
	
	if (ImGui::Button("Save")) {
		items.save(no::asset_path("items.data"));
	}

	ImGui::End();
	no::imgui::end_frame();
}

void item_editor::draw() {
	no::imgui::draw();
}
