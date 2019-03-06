#pragma once

#include "item.hpp"

#include "imgui/imgui.h"
#include "imgui/imgui_platform.h"

#include <string>

no::vector2f item_uv1(no::vector2f uv, int texture);
no::vector2f item_uv2(no::vector2f uv, int texture);

template<int Size>
bool imgui_input_text(const std::string& label, std::string& value) {
	char buffer[Size] = {};
	strcpy_s(buffer, value.c_str());
	bool changed = ImGui::InputText(label.c_str(), buffer, Size);
	value = buffer;
	return changed;
}

template<int Size>
bool imgui_input_text_multiline(const std::string& label, std::string& value, no::vector2f box_size) {
	char buffer[Size] = {};
	strcpy_s(buffer, value.c_str());
	bool changed = ImGui::InputTextMultiline(label.c_str(), buffer, Size, box_size);
	value = buffer;
	return changed;
}

void imgui_draw_grid(ImDrawList* draw_list, no::vector2f offset);
bool item_popup_context(std::string label, item_instance* out_item, int texture, bool& dirty);
bool select_stat_combo(int* stat);
bool select_quest_combo(int* quest_id);
bool select_quest_task_combo(int quest_id, int* task_id);
