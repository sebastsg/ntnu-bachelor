#pragma once

#include "imgui/imgui.h"

#include <string>

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
