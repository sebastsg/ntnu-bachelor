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
	load_texture();
	if (vertex_type == animated) {
		model.load(data);
	} else if (vertex_type == static_textured) {
		model.load(data.to<no::static_textured_vertex>([](const no::animated_mesh_vertex& vertex) {
			return no::static_textured_vertex{ vertex.position, vertex.tex_coords };
		}));
	}
	instance = { model };
	if (vertex_type == 0 && model.total_animations() > 0) {
		instance.start_animation(0);
	}
	this->data.texture.reserve(100);
	this->name.reserve(100);
}

loaded_model& loaded_model::operator=(loaded_model&& that) {
	std::swap(name, that.name);
	std::swap(model, that.model);
	std::swap(data, that.data);
	std::swap(vertex_type, that.vertex_type);
	std::swap(texture, that.texture);
	instance = { model };
	if (vertex_type == animated && model.total_animations() > 0) {
		instance.start_animation(0);
	}
	return *this;
}

void loaded_model::load_texture() {
	if (texture != -1) {
		no::delete_texture(texture);
	}
	std::string path = no::asset_path("textures/" + data.texture);
	if (std::filesystem::path(path).extension() == "") {
		path += ".png";
	}
	texture = no::create_texture(no::surface(path), no::scale_option::nearest_neighbour, true);
}

void loaded_model::draw() {
	if (model.is_drawable() && model.total_animations() > 0) {
		instance.animate();
	}
	if (texture != -1) {
		no::bind_texture(texture);
	}
	if (model.is_drawable()) {
		instance.draw();
	}
}

int loaded_model::type() const {
	return vertex_type;
}

void loaded_model::scale_vertices(no::vector3f scale) {
	for (auto& vertex : data.shape.vertices) {
		vertex.position *= scale;
	}
	model.load(data);
}

converter_tool::converter_tool() {
	
}

void converter_tool::update() {
	ImGui::Text("Converter");

	ImGui::Checkbox("Add default bone", &import_options.add_bone);
	ImGui::Text("Import model type:");
	ImGui::RadioButton("Animated", &import_options.vertex_type, loaded_model::animated);
	ImGui::RadioButton("Static, textured", &import_options.vertex_type, loaded_model::static_textured);

	if (ImGui::Button("Load OBJ or COLLADA file")) {
		std::vector<no::model_data<no::animated_mesh_vertex>> models;
		std::string browsed_path = no::platform::open_file_browse_window();
		no::model_conversion_options options;
		options.import.bones = { import_options.add_bone, model.name };
		options.exporter = [&](const std::string& destination, const no::model_data<no::animated_mesh_vertex>& data) {
			models.push_back(data);
		};
		auto path = std::filesystem::path(browsed_path).parent_path();
		auto files = no::entries_in_directory(path.string(), no::entry_inclusion::only_files);
		for (auto& file : files) {
			auto extension = no::file_extension_in_path(file);
			if (extension != ".dae" && extension != ".obj") {
				continue;
			}
			std::string name = std::filesystem::path(file).stem().string();
			no::convert_model(file, no::asset_path("models/decorations/" + name + ".nom"), options);
		}
		auto merged_model = no::merge_model_animations<no::animated_mesh_vertex>(models);
		model = { std::filesystem::path(browsed_path).stem().string(), import_options.vertex_type, merged_model };
		if (models.size() > 1) {
			model.name = std::filesystem::path(browsed_path).parent_path().stem().string();
		}
	}

	ImGui::Separator();

	float scale = transform.scale.x;
	ImGui::InputFloat("Scale", &scale, 0.01f, 0.1f, 2);
	scale = std::min(std::max(scale, 0.01f), 10.0f);
	transform.scale = scale;
	if (ImGui::Button("Apply scale")) {
		model.scale_vertices(transform.scale);
		transform.scale = 1.0f;
	}
	ImGui::Text("Texture");
	ImGui::SameLine();
	ImGui::InputText("##TextureName", model.data.texture.data(), model.data.texture.capacity());
	ImGui::SameLine();
	if (ImGui::Button("Apply##ApplyTexture")) {
		model.data.texture = model.data.texture.data();
		model.load_texture();
	}

	ImGui::Separator();

	node_names_for_list.clear();
	for (auto& node : model.data.nodes) {
		node_names_for_list.push_back(std::string(node.depth, '.') + node.name);
	}
	ImGui::ListBox("##NodeList", &current_node, [](void* data, int i, const char** out) -> bool {
		auto& self = *(converter_tool*)data;
		if (i < 0 || i >= (int)self.model.data.nodes.size()) {
			return false;
		}
		*out = self.node_names_for_list[i].c_str();
		return true;
	}, this, model.data.nodes.size(), 10);
	if (current_node >= 0 && (int)model.data.nodes.size() > current_node) {
		auto& node = model.data.nodes[current_node];
		ImGui::Text(CSTRING("Name: " << node.name));
		if (!model.data.animations.empty()) {
			auto& reference_channel = model.data.animations[0].channels[current_node];
			if (ImGui::BeginCombo("##ChannelBone", CSTRING(reference_channel.bone))) {
				if (ImGui::Selectable("None")) {
					reference_channel.bone = -1;
				}
				for (int bone = 0; bone < (int)model.data.bones.size(); bone++) {
					if (ImGui::Selectable(CSTRING(bone << ": " << model.data.bone_names[bone]))) {
						reference_channel.bone = bone;
					}
				}
				ImGui::EndCombo();
			}
			for (auto& animation : model.data.animations) {
				animation.channels[current_node].bone = reference_channel.bone;
			}
		}
	}

	ImGui::Separator();

	int old_animation = model.animation;
	ImGui::ListBox("##SelectAnimation", &model.animation, [](void* data, int i, const char** out) -> bool {
		auto& model = *(no::model*)data;
		if (i < 0 || i >= model.total_animations()) {
			return false;
		}
		*out = model.animation(i).name.c_str();
		return true;
	}, &model.model, model.model.total_animations(), 10);
	if (old_animation != model.animation) {
		model.instance.start_animation(model.animation);
	}

	ImGui::Separator();

	ImGui::Text("Export as ");
	ImGui::SameLine();
	ImGui::InputText("##ExportModelName", model.name.data(), model.name.capacity());
	if (ImGui::Button("Export NOM model")) {
		model.name = model.name.data();
		std::string path = no::asset_path("models/" + model.name + ".nom");
		if (!overwrite_export) {
			while (std::filesystem::exists(path)) {
				path.insert(path.end() - 4, '_');
			}
		}
		no::export_model(path, model.data);
	}
	ImGui::SameLine();
	ImGui::Checkbox("Overwrite file", &overwrite_export);
}

void converter_tool::draw() {
	no::set_shader_model(transform);
	model.draw();
}

void attachments_tool::update() {
	ImGui::Text("Attachments");

	if (!main_model.is_drawable() && ImGui::Button("Import main NOM model")) {
		std::string browsed_path = no::platform::open_file_browse_window();
		main_model.load<no::animated_mesh_vertex>(browsed_path);
		instance = { main_model };
	}

	if (main_model.is_drawable() && ImGui::Button("Import attachment NOM model")) {
		std::string browsed_path = no::platform::open_file_browse_window();
		auto& attachment = attachments.emplace_back();
		attachment.name = std::filesystem::path(browsed_path).stem().string();
		attachment.model.load<no::animated_mesh_vertex>(browsed_path);
		attachment.channel = 15;
		attachment.position = { 0.0761f, 0.5661f, 0.1151f };
		attachment.rotation = { 0.595f, -0.476f, -0.464f, -0.452f };
		attachment.id = instance.attach(attachment.channel, attachment.model, attachment.position, attachment.rotation);
	}

	ImGui::Text("Loaded attachment models");

	int old_attachment = current_attachment;
	ImGui::ListBox("##AttachmentList", &current_attachment, [](void* data, int i, const char** out) -> bool {
		auto& attachments = *(std::vector<attachment_data>*)data;
		if (i < 0 || i >= (int)attachments.size()) {
			return false;
		}
		*out = attachments[i].name.data();
		return true;
	}, &attachments, attachments.size(), 10);
	if (attachments.size() > 0) {
		auto& attachment = attachments[current_attachment];
		ImGui::Separator();
		int old_channel = attachment.channel;
		ImGui::ListBox("##MainNodeList", &attachment.channel, [](void* data, int i, const char** out) -> bool {
			auto& model = *(no::model*)data;
			if (i < 0 || i >= model.total_nodes()) {
				return false;
			}
			*out = model.node(i).name.data();
			return true;
		}, &main_model, main_model.total_nodes(), 10);
		if (old_channel != attachment.channel) {
			instance.detach(attachment.id);
			attachment.id = instance.attach(attachment.channel, attachment.model, attachment.position, attachment.rotation);
		}
		ImGui::Text(attachment.name.data());
		ImGui::Text(CSTRING("Attached to node " << main_model.node(attachment.channel).name << " (" << attachment.channel << ")"));
		ImGui::Text("Position");
		ImGui::SameLine();
		ImGui::InputFloat3("##AttachmentPosition", &attachment.position.x, 5);
		ImGui::Text("Rotation");
		ImGui::SameLine();
		float wxyz[4] = { attachment.rotation.w, attachment.rotation.x, attachment.rotation.y, attachment.rotation.z };
		ImGui::InputFloat4("##AttachmentRotation", wxyz, 5);
		attachment.rotation = { wxyz[0], wxyz[1], wxyz[2], wxyz[3] };
		instance.set_attachment_bone(attachment.id, attachment.position, attachment.rotation);
	}

	ImGui::Separator();

	ImGui::Text("View animation");
	int old_animation = animation;
	ImGui::ListBox("##ViewAnimationList", &animation, [](void* data, int i, const char** out) -> bool {
		auto& model = *(no::model*)data;
		if (i < 0 || i >= model.total_animations()) {
			return false;
		}
		*out = model.animation(i).name.data();
		return true;
	}, &main_model, main_model.total_animations(), 10);
	if (old_animation != animation) {
		instance.start_animation(animation);
	}

	ImGui::Separator();

	if (ImGui::Button("Save attachment details")) {

	}

}

void attachments_tool::draw() {
	if (!instance.can_animate()) {
		return;
	}
	no::set_shader_model({});
	instance.animate();
	if (texture != -1) {
		no::bind_texture(texture);
	}
	instance.draw();
}

model_manager_state::model_manager_state() : dragger(mouse()) {
	no::imgui::create(window());
	mouse_scroll_id = mouse().scroll.listen([this](const no::mouse::scroll_message& event) {
		if (is_mouse_over_ui()) {
			return;
		}
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
	animate_shader = no::create_shader(no::asset_path("shaders/diffuse"));
	static_shader = no::create_shader(no::asset_path("shaders/static_textured"));
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
	ImGui::SetNextWindowSize({ 480.0f, (float)window().height() }, ImGuiSetCond_Always);
	ImGui::Begin("##ModelManager", nullptr, imgui_flags);
	ImGui::Text(CSTRING("FPS: " << frame_counter().current_fps()));
	ImGui::Separator();

	ImGui::Text("Current tool:");
	ImGui::RadioButton("Converter", &current_tool, 0);
	ImGui::RadioButton("Attachments", &current_tool, 1);
	ImGui::Separator();

	if (current_tool == 0) {
		converter.update();
	} else if (current_tool == 1) {
		attachments.update();
	}

	ImGui::Separator();

	ImGui::End();
	no::imgui::end_frame();

	if (!is_mouse_over_ui()) {
		dragger.update(camera);
	}
	camera.update();
}

void model_manager_state::draw() {
	if (current_tool == 0) {
		if (converter.model.type() == 0) {
			no::bind_shader(animate_shader);
			no::get_shader_variable("uni_Color").set({ 1.0f, 1.0f, 1.0f, 1.0f });
			no::get_shader_variable("uni_LightPosition").set(camera.transform.position + camera.offset());
			no::get_shader_variable("uni_LightColor").set(no::vector3f{ 1.0f, 1.0f, 1.0f });
		} else {
			no::bind_shader(static_shader);
		}
		no::set_shader_view_projection(camera);
		converter.draw();
	} else if (current_tool == 1) {
		no::bind_shader(animate_shader);
		no::get_shader_variable("uni_Color").set({ 1.0f, 1.0f, 1.0f, 1.0f });
		no::get_shader_variable("uni_LightPosition").set(camera.transform.position + camera.offset());
		no::get_shader_variable("uni_LightColor").set(no::vector3f{ 1.0f, 1.0f, 1.0f });
		no::set_shader_view_projection(camera);
		attachments.draw();
	}
	no::imgui::draw();
}

bool model_manager_state::is_mouse_over_ui() const {
	return mouse().position().x < 480;
}
