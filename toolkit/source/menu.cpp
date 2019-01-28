#include "menu.hpp"
#include "model_manager.hpp"
#include "editor.hpp"
#include "item_editor.hpp"

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
		if (ImGui::MenuItem("Item editor")) {
			change_state<item_editor>();
		}
		if (ImGui::MenuItem("Object editor")) {

		}
		if (ImGui::MenuItem("Dialogue editor")) {

		}
		ImGui::EndMenu();
	}
	ImGui::EndMainMenuBar();
}

void menu_bar_state::draw() {

}
