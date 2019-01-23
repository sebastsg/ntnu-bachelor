#include "model_manager.hpp"

#include "platform.hpp"
#include "assets.hpp"
#include "window.hpp"
#include "surface.hpp"

#include "imgui/imgui.h"
#include "imgui/imgui_platform.h"

#include <filesystem>

loaded_model::loaded_model(const std::string& name, int vertex_type, const no::model_data<no::animated_mesh_vertex>& data) 
	: vertex_type(vertex_type), data(data), name(name) {
	texture = no::create_texture(no::surface(no::asset_path("textures/decorations.png")), no::scale_option::nearest_neighbour, true);
	if (vertex_type == 0) {
		model.load(data);
	} else if (vertex_type == 1) {
		model.load(data.to<no::static_textured_vertex>([](const no::animated_mesh_vertex& vertex) {
			return no::static_textured_vertex{ vertex.position, vertex.tex_coords };
		}));
	}
	instance.set_source(model);
	if (vertex_type == 0 && model.total_animations() > 0) {
		instance.start_animation(0);
	}
}

loaded_model::loaded_model(loaded_model&& that) {
	std::swap(name, that.name);
	std::swap(model, that.model);
	instance.set_source(model);
	std::swap(data, that.data);
	std::swap(vertex_type, that.vertex_type);
	std::swap(texture, that.texture);
}

loaded_model& loaded_model::operator=(loaded_model&& that) {
	std::swap(name, that.name);
	std::swap(model, that.model);
	instance.set_source(model);
	std::swap(data, that.data);
	std::swap(vertex_type, that.vertex_type);
	std::swap(texture, that.texture);
	return *this;
}

void loaded_model::draw() {
	if (model.is_drawable() && model.total_animations() > 0) {
		instance.animate();
	}
	no::bind_texture(texture);
	if (model.is_drawable()) {
		instance.bind();
		instance.draw();
	}
}

model_manager_state::model_manager_state() : dragger(mouse()) {
	no::imgui::create(window());
	animate_shader = no::create_shader(no::asset_path("shaders/diffuse"));
	static_shader = no::create_shader(no::asset_path("shaders/static_textured"));
	mouse_scroll_id = mouse().scroll.listen([this](const no::mouse::scroll_message& event) {
		float& factor = camera.rotation_offset_factor;
		factor -= (float)event.steps;
		if (factor > 12.0f) {
			factor = 12.0f;
		} else if (factor < 4.0f) {
			factor = 4.0f;
		}
	});
	camera.transform.position.y = 1.0f;
	camera.rotation_offset_factor = 12.0f;
	models.reserve(100);
}

model_manager_state::~model_manager_state() {
	mouse().scroll.ignore(mouse_scroll_id);
	no::imgui::destroy();
}

void model_manager_state::update() {
	camera.size = window().size().to<float>();
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

	ImGui::Checkbox("Add default bone", &import_options.add_bone);
	ImGui::InputText("Bone name", import_options.name, 64, import_options.add_bone ? 0 : ImGuiInputTextFlags_ReadOnly);
	ImGui::Text("Import model type:");
	ImGui::RadioButton("Animation", &import_options.vertex_type, 0);
	ImGui::RadioButton("Static, textured", &import_options.vertex_type, 1);

	if (ImGui::Button("Load OBJ or COLLADA file")) {
		std::string path = no::platform::open_file_browse_window();
		no::model_conversion_options options;
		options.import.bones = { import_options.add_bone, import_options.name };
		options.exporter = [this](const std::string& destination, const no::model_data<no::animated_mesh_vertex>& data) {
			models.emplace_back(destination, import_options.vertex_type, data);
		};
		no::convert_model(path, std::filesystem::path(path).stem().string(), options);
	}

	ImGui::Separator();

	float scale = model_transform.scale.x;
	ImGui::InputFloat("Scale", &scale, 0.01f, 0.1f, 2);
	scale = std::min(std::max(scale, 0.01f), 10.0f);
	model_transform.scale = scale;

	ImGui::ListBox("##LoadedModels", &active_model, [](void* data, int i, const char** out) -> bool {
		auto& paths = *(std::vector<loaded_model>*)data;
		if (i < 0 || i >= (int)paths.size()) {
			return false;
		}
		*out = paths[i].name.c_str();
		return true;
	}, &models, models.size(), 20);

	ImGui::Separator();

	ImGui::Separator();

	if (ImGui::Button("Export as nom file")) {
		
	}

	ImGui::Separator();

	ImGui::End();
	no::imgui::end_frame();

	dragger.update(camera);
	camera.update();
}

void model_manager_state::draw() {
	int i = 0;
	for (auto& model : models) {
		if (i++ != active_model) {
			continue;
		}
		if (model.type() == 0) {
			no::bind_shader(animate_shader);
			no::get_shader_variable("uni_Color").set({ 1.0f, 1.0f, 1.0f, 1.0f });
			no::get_shader_variable("uni_LightPosition").set(camera.transform.position + camera.offset());
			no::get_shader_variable("uni_LightColor").set(no::vector3f{ 1.0f, 1.0f, 1.0f });
		} else {
			no::bind_shader(static_shader);
		}
		no::set_shader_view_projection(camera);
		no::set_shader_model(model_transform);
		model.draw();
	}
	no::imgui::draw();
}
