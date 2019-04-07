#include "item_editor.hpp"
#include "window.hpp"
#include "assets.hpp"
#include "surface.hpp"

#include "imgui/imgui.h"
#include "imgui/imgui_platform.h"

item_editor::item_editor() {
	no::imgui::create(window());
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

void item_editor::equipment_slot_combo(equipment_slot& target, const std::string& ui_id) {
	if (ImGui::BeginCombo(CSTRING("Equipment slot##ItemSlot" << ui_id), CSTRING(target))) {
		for (int slot_value = 0; slot_value < (int)equipment_slot::total_slots; slot_value++) {
			equipment_slot slot = (equipment_slot)slot_value;
			if (ImGui::Selectable(CSTRING(slot << "##ItemSlot" << ui_id << slot_value))) {
				target = slot;
			}
		}
		ImGui::End();
	}
}

void item_editor::equipment_type_combo(equipment_type& target, const std::string& ui_id) {
	if (ImGui::BeginCombo(CSTRING("Equipment type##ItemEquipType" << ui_id), CSTRING(target))) {
		for (int type_value = 0; type_value < (int)equipment_type::total_types; type_value++) {
			equipment_type type = (equipment_type)type_value;
			if (ImGui::Selectable(CSTRING(type << "##ItemEquipType" << ui_id << type_value))) {
				target = type;
			}
		}
		ImGui::End();
	}
}

void item_editor::ui_create_item() {
	if (ImGui::CollapsingHeader("Create new item")) {
		imgui_input_text<128>("Name##NewItemName", new_item.name);
		ImGui::InputScalar("Max stack##NewItemMaxStack", ImGuiDataType_S64, &new_item.max_stack);
		new_item.max_stack = std::max(1LL, new_item.max_stack);
		imgui_input_text<128>("Model##NewItemModel", new_item.model);
		item_type_combo(new_item.type, "New");
		equipment_slot_combo(new_item.slot, "New");
		if (ImGui::Button("Save##SaveNewItem")) {
			item_definition item = new_item;
			item.id = item_definitions().count();
			item_definitions().add(item);
		}
	}
}

void item_editor::ui_select_item() {
	ImGui::Text("Select item:");
	ImGui::SameLine();
	if (ImGui::BeginCombo("##SelectEditItem", item_definitions().get(current_item).name.c_str())) {
		for (int id = 0; id < item_definitions().count(); id++) {
			if (ImGui::Selectable(item_definitions().get(id).name.c_str())) {
				current_item = id;
			}
		}
		ImGui::EndCombo();
	}
	auto& selected = item_definitions().get(current_item);
	if (selected.id == -1) {
		return;
	}
	ImGui::Text(CSTRING("ID: " << selected.id));
	imgui_input_text<64>("Name##EditItemName", selected.name);
	ImGui::InputScalar("Max stack##EditItemMaxStack", ImGuiDataType_S64, &selected.max_stack);
	selected.max_stack = std::max(1LL, selected.max_stack);
	item_type_combo(selected.type, "Edit");
	equipment_slot_combo(selected.slot, "Edit");
	equipment_type_combo(selected.equipment, "Edit");
	std::string uv_name[] = { "Default", "Tiny", "Small", "Medium", "Large" };
	int i = 0;
	for (no::vector2f* uv = &selected.uv; uv <= &selected.uv_large_stack; uv++) {
		ImGui::InputFloat2(CSTRING("UV (" << uv_name[i] << ")##EditItemUV"), &uv->x, 0);
		no::vector2f uv_start = *uv / no::texture_size(ui_texture).to<float>();
		no::vector2f uv_end = (*uv + 32.0f) / no::texture_size(ui_texture).to<float>();
		ImGui::Image((ImTextureID)ui_texture, { 64.0f, 64.0f }, { uv_start }, { uv_end }, { 1.0f }, { 1.0f });
		i++;
	}
	imgui_input_text<64>("Model##EditItemModel", selected.model);
	ImGui::Checkbox("Is attachment##EditIsAttachment", &selected.attachment);
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
		item_definitions().save(no::asset_path("items.data"));
	}

	ImGui::End();
	no::imgui::end_frame();
}

void item_editor::draw() {
	no::imgui::draw();
}
