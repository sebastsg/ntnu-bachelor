#include "model_manager.hpp"

#include "platform.hpp"
#include "window.hpp"

#include "imgui/imgui.h"
#include "imgui/imgui_platform.h"

model_manager_state::model_manager_state() : instance(model) {
	no::imgui::create(window());
}

model_manager_state::~model_manager_state() {
	no::imgui::destroy();
}

void model_manager_state::update() {
	no::imgui::start_frame();
	ImGuiWindowFlags imgui_flags
		= ImGuiWindowFlags_NoMove
		| ImGuiWindowFlags_NoResize
		| ImGuiWindowFlags_NoCollapse
		| ImGuiWindowFlags_NoTitleBar;
	ImGui::SetNextWindowPos({ 0.0f, 0.0f }, ImGuiSetCond_Once);
	ImGui::SetNextWindowSize({ 320.0f, (float)window().height() }, ImGuiSetCond_Always);
	ImGui::Begin("##ModelManager", nullptr, imgui_flags);
	ImGui::Text(CSTRING("FPS: " << frame_counter().current_fps()));
	ImGui::Separator();

	if (ImGui::Button("Load OBJ or COLLADA file")) {
		std::string path = no::platform::open_file_browse_window();
		no::model_conversion_options options;
		options.exporter = [this](const std::string& destination, const no::model_data<no::animated_mesh_vertex>& data) {
			model_data = data;
			model.load(data);
			instance = { model };
		};
		no::convert_model(path, "", options);
	}

	if (ImGui::Button("Export as nom file")) {
		
	}

	ImGui::End();
	no::imgui::end_frame();
}

void model_manager_state::draw() {
	no::bind_shader(shader);
	no::set_shader_view_projection(camera);
	
	no::bind_texture(model_texture);
	instance.bind();
	instance.draw();

	no::imgui::draw();
}
