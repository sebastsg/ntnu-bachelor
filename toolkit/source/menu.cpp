#include "menu.hpp"
#include "model_manager.hpp"
#include "editor.hpp"

#include "imgui/imgui.h"

void menu_bar_state::update() {
	ImGui::BeginMainMenuBar();
	if (ImGui::BeginMenu("Tool")) {
		if (ImGui::MenuItem("World editor")) {
			change_state<world_editor_state>();
		}
		if (ImGui::MenuItem("Model manager")) {
			change_state<model_manager_state>();
		}
		if (ImGui::MenuItem("Item creator")) {

		}
		if (ImGui::MenuItem("Object creator")) {

		}
		ImGui::EndMenu();
	}
	ImGui::EndMainMenuBar();
}

void menu_bar_state::draw() {

}
