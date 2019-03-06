#include "common_ui.hpp"
#include "quest.hpp"
#include "draw.hpp"

no::vector2f item_uv1(no::vector2f uv, int texture) {
	return uv / no::texture_size(texture).to<float>();
}

no::vector2f item_uv2(no::vector2f uv, int texture) {
	return (uv + 32.0f) / no::texture_size(texture).to<float>();
}

void imgui_draw_grid(ImDrawList* draw_list, no::vector2f offset) {
	const float grid_size = 32.0f;
	const ImColor line_color{ 200, 200, 200, 40 };
	no::vector2f win_pos = ImGui::GetCursorScreenPos();
	no::vector2f canvas_size = ImGui::GetWindowSize();
	for (float x = std::fmodf(offset.x, grid_size); x < canvas_size.x; x += grid_size) {
		no::vector2f pos1 = { x + win_pos.x, win_pos.y };
		no::vector2f pos2 = { x + win_pos.x, canvas_size.y + win_pos.y };
		draw_list->AddLine(pos1, pos2, line_color);
	}
	for (float y = std::fmodf(offset.y, grid_size); y < canvas_size.y; y += grid_size) {
		no::vector2f pos1 = { win_pos.x, y + win_pos.y };
		no::vector2f pos2 = { canvas_size.x + win_pos.x, y + win_pos.y };
		draw_list->AddLine(pos1, pos2, line_color);
	}
}

bool item_popup_context(std::string imgui_id, item_instance* out_item, int texture, bool& dirty) {
	bool opened = false;
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
	ImGui::PushID(CSTRING("ItemContext" << imgui_id));
	if (ImGui::BeginPopupContextItem(CSTRING(imgui_id << out_item))) {
		for (int type_num = 0; type_num < (int)item_type::total_types; type_num++) {
			ImGui::PushID(CSTRING("Type" << type_num));
			if (ImGui::BeginMenu(CSTRING((item_type)type_num))) {
				auto definitions = item_definitions().of_type((item_type)type_num);
				for (auto& definition : definitions) {
					ImGui::Image((ImTextureID)texture, { 12.0f }, item_uv1(definition.uv, texture), item_uv2(definition.uv, texture));
					ImGui::SameLine();
					if (ImGui::MenuItem(CSTRING(definition.name << "##Item" << definition.id))) {
						out_item->definition_id = definition.id;
						out_item->stack = (int)std::min((int)definition.max_stack, (int)out_item->stack);
						dirty = true;
					}
				}
				ImGui::EndMenu();
			}
			ImGui::PopID();
		}
		if (ImGui::MenuItem(CSTRING("Nothing##NoItem" << imgui_id))) {
			out_item->definition_id = -1;
			out_item->stack = 0;
		}
		if (out_item->definition_id != -1) {
			int stack = (int)out_item->stack;
			ImGui::PushItemWidth(100.0f);
			dirty |= ImGui::InputInt("Stack", &stack);
			ImGui::PopItemWidth();
			out_item->stack = (int64_t)std::max(1, stack);
		} else {
			out_item->stack = 0;
		}
		ImGui::EndPopup();
		opened = true;
	}
	ImGui::PopID();
	ImGui::PopStyleVar();
	return opened;
}

bool select_stat_combo(int* stat) {
	return ImGui::Combo(CSTRING("Stat##" << stat), stat, "Sword\0Axe\0Spear\0Defense\0Archery\0Stamina\0Fishing\0\0");
}

bool select_quest_combo(int* quest_id) {
	bool changed = false;
	auto current_quest = quest_definitions().find(*quest_id);
	if (ImGui::BeginCombo("Quest", current_quest ? current_quest->name.c_str() : "No quest selected")) {
		for (int i = 0; i < quest_definitions().count(); i++) {
			auto quest = quest_definitions().find_by_index(i);
			if (ImGui::Selectable(CSTRING(quest->name))) {
				*quest_id = quest->id;
				changed = true;
			}
		}
		ImGui::EndCombo();
	}
	return changed;
}

bool select_quest_task_combo(int quest_id, int* task_id) {
	auto quest = quest_definitions().find(quest_id);
	if (!quest) {
		return false;
	}
	bool changed = false;
	auto current_task = quest->tasks.find(*task_id);
	if (ImGui::BeginCombo("Task", current_task ? current_task->name.c_str() : "No task selected")) {
		for (int i = 0; i < quest->tasks.count(); i++) {
			auto task = quest->tasks.find_by_index(i);
			if (ImGui::Selectable(CSTRING(task->name))) {
				*task_id = task->id;
				changed = true;
			}
		}
		ImGui::EndCombo();
	}
	return changed;
}
