#include "quest_editor.hpp"
#include "window.hpp"
#include "surface.hpp"

quest_editor::quest_editor() {
	no::imgui::create(window());
	window().set_clear_color(0.2f);
	blank_texture = no::create_texture(no::surface{ 2, 2, no::pixel_format::rgba, 0xFFFFFFFF });
}

quest_editor::~quest_editor() {
	no::delete_texture(blank_texture);
	no::imgui::destroy();
}

void quest_editor::update() {
	no::imgui::start_frame();
	menu_bar_state::update();
	next_window_x = 0.0f;

	begin_window("##QuestList", 320.0f);
	update_quest_list();
	ImGui::Separator();
	if (ImGui::Button("Save")) {
		quest_definitions().save();
	}
	end_window();

	begin_window("##QuestTasks", 512.0f);
	update_tasks();
	end_window();

	begin_window("##QuestHints", 640.0f);
	update_hints();
	end_window();

	no::imgui::end_frame();
}

void quest_editor::draw() {
	no::imgui::draw();
}

void quest_editor::begin_window(const std::string& label, float width) {
	ImGuiWindowFlags imgui_flags
		= ImGuiWindowFlags_NoMove
		| ImGuiWindowFlags_NoResize
		| ImGuiWindowFlags_NoCollapse
		| ImGuiWindowFlags_NoTitleBar;
	ImGui::SetNextWindowPos({ next_window_x, 20.0f }, ImGuiSetCond_Once);
	ImGui::SetNextWindowSize({ width, (float)window().height() - 20.0f }, ImGuiSetCond_Always);
	ImGui::Begin(label.c_str(), nullptr, imgui_flags);
	next_window_x += width;
}

void quest_editor::end_window() {
	ImGui::End();
}

void quest_editor::create_new_quest() {
	int id = quest_definitions().add("New quest").id;
	selected_quest_index = quest_definitions().find_index_by_id(id);
}

void quest_editor::load_quest(int index) {
	selected_quest_index = -1;
	auto found_quest = quest_definitions().find_by_index(index);
	if (found_quest) {
		selected_quest_index = index;
	}
}

void quest_editor::save_quest() {
	quest_definitions().save();
	dirty = false;
}

void quest_editor::update_quest_list() {
	int new_quest_index = selected_quest_index;
	ImGui::PushItemWidth(-1);
	ImGui::ListBox("##SelectQuest", &new_quest_index, [](void* data, int i, const char** out_text) -> bool {
		auto item = quest_definitions().find_by_index(i);
		if (!item) {
			return false;
		}
		*out_text = &item->name[0];
		return true;
	}, this, quest_definitions().count(), 20);
	ImGui::PopItemWidth();
	if (new_quest_index != selected_quest_index) {
		save_quest();
		load_quest(new_quest_index);
	}
	if (ImGui::Button("Create new quest")) {
		create_new_quest();
	}
	ImGui::Separator();

	if (selected_quest_index == -1) {
		return;
	}
	auto selected = quest_definitions().find_by_index(selected_quest_index);
	if (!selected) {
		return;
	}
	ImGui::Text("Name:");
	ImGui::SameLine();
	imgui_input_text<128>("##QuestName", selected->name);
	ImGui::Text("Quest ID: %i", selected->id);
}

void quest_editor::update_tasks() {
	if (selected_quest_index == -1) {
		return;
	}
	auto quest = quest_definitions().find_by_index(selected_quest_index);
	for (int task_index = 0; task_index < quest->tasks.count(); task_index++) {
		auto task = quest->tasks.find_by_index(task_index);
		ImGui::PushID(CSTRING("QuestTask" << task_index));
		ImGui::BeginGroup();
		ImGui::Text(CSTRING("ID: " << task->id));
		ImGui::SameLine();
		imgui_input_text<64>("##TaskName", task->name);
		ImGui::Checkbox("Optional", &task->optional);
		ImGui::PushItemWidth(128.0f);
		ImGui::InputInt("Goal", &task->goal);
		ImGui::PopItemWidth();
		if (ImGui::Button("Delete")) {
			quest->tasks.remove(task->id);
			task_index--;
		}
		ImGui::EndGroup();
		ImGui::PopID();
		ImGui::Separator();
	}
	if (ImGui::Button("Add task")) {
		quest->tasks.add("New task");
	}
}

void quest_editor::update_hints() {
	if (selected_quest_index == -1) {
		return;
	}
	auto quest = quest_definitions().find_by_index(selected_quest_index);
	for (int i = 0; i < quest->hints.size(); i++) {
		auto& hint = quest->hints[i];
		ImGui::PushID(CSTRING("QuestHint" << i));
		ImGui::BeginGroup();
		ImGui::Text("Unlocked by: ");
		ImGui::SameLine();
		ImGui::PushItemWidth(128.0f);
		ImGui::InputInt("##UnlockedByTask", &hint.unlocked_by_task_id);
		ImGui::PopItemWidth();
		ImGui::SameLine();
		auto task = quest->tasks.find(hint.unlocked_by_task_id);
		if (task) {
			ImGui::Text(task->name.c_str());
			ImGui::SameLine();
			ImGui::Image((ImTextureID)blank_texture, { 16.0f }, { 0.0f }, { 1.0f }, { 0.2f, 1.0f, 0.4f, 0.7f });
		} else {
			ImGui::Text("No task required.");
			ImGui::SameLine();
			ImGui::Image((ImTextureID)blank_texture, { 16.0f }, { 0.0f }, { 1.0f }, { 0.8f, 0.2f, 0.2f, 0.7f });
			hint.unlocked_by_task_id = -1;
		}
		ImGui::Text("Message");
		imgui_input_text_multiline<512>("##HintMessage", hint.message, { -1.0f, 128.0f });
		if (ImGui::Button("Delete")) {
			quest->hints.erase(quest->hints.begin() + i);
			i--;
		}
		ImGui::EndGroup();
		ImGui::PopID();
		ImGui::Separator();
	}
	if (ImGui::Button("Add hint")) {
		quest->hints.emplace_back();
	}
}
