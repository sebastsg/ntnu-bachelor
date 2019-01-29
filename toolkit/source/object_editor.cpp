#include "object_editor.hpp"
#include "window.hpp"
#include "assets.hpp"

#include "imgui/imgui.h"
#include "imgui/imgui_platform.h"

object_editor::object_editor() {
	no::imgui::create(window());
}

object_editor::~object_editor() {
	no::imgui::destroy();
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



	ImGui::End();
	no::imgui::end_frame();
}

void object_editor::draw() {
	no::imgui::draw();
}

