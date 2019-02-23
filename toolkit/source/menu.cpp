#include "menu.hpp"
#include "model_manager.hpp"
#include "editor.hpp"
#include "item_editor.hpp"
#include "object_editor.hpp"
#include "dialogue_editor.hpp"
#include "window.hpp"

#include "imgui/imgui.h"

void menu_bar_state::update() {
	ImGui::BeginMainMenuBar();
	update_menu();
	ImGui::EndMainMenuBar();
}

void menu_bar_state::draw() {

}

void menu_bar_state::update_menu() {
	if (ImGui::BeginMenu("Tool")) {
		ImGui::PushItemWidth(360.0f);
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
			change_state<object_editor>();
		}
		if (ImGui::MenuItem("Dialogue editor")) {
			change_state<dialogue_editor_state>();
		}
		ImGui::PopItemWidth();
		ImGui::EndMenu();
	}
	if (ImGui::BeginMenu("Options")) {
		ImGui::PushItemWidth(360.0f);
		if (ImGui::MenuItem("Limit FPS", nullptr, &limit_fps)) {
			set_synchronization(limit_fps ? no::draw_synchronization::if_updated : no::draw_synchronization::always);
		}
		ImGui::PopItemWidth();
		ImGui::EndMenu();
	}
}
