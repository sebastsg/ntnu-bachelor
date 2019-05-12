#include "model_manager.hpp"
#include "render.hpp"

#include "platform.hpp"
#include "assets.hpp"
#include "window.hpp"
#include "surface.hpp"

#include "imgui/imgui.h"
#include "imgui/imgui_platform.h"
#include "imgui/imgui_internal.h"

#include <filesystem>

void loaded_model::load(const std::string& name, int vertex_type, const no::model_data<no::animated_mesh_vertex>& data) {
	this->name = name;
	this->vertex_type = vertex_type;
	this->data = data;
	load_texture();
	if (vertex_type == animated) {
		model.load(data);
	} else if (vertex_type == static_object) {
		model.load(data.to<static_object_vertex>([](const no::animated_mesh_vertex& vertex) {
			return static_object_vertex{ vertex.position, vertex.normal, vertex.tex_coords };
		}));
	}
	animator.add();
	if (vertex_type == 0 && model.total_animations() > 0) {
		animator.play(0, 0, -1);
	}
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
		animator.animate();
	}
	if (texture != -1) {
		no::bind_texture(texture);
	}
	if (model.is_drawable()) {
		if (model.total_animations() > 0) {
			animator.shader.bones = no::get_shader_variable("uni_Bones");
			animator.draw();
		} else {
			model.bind();
			model.draw();
		}
	}
}

int loaded_model::type() const {
	return vertex_type;
}

void loaded_model::scale_vertices(no::vector3f scale) {
	for (auto& vertex : data.shape.vertices) {
		vertex.position *= scale;
	}
	data.min *= scale;
	data.max *= scale;
	model.load(data);
}

converter_tool::converter_tool() {
	
}

void converter_tool::update() {
	ImGui::Text("Converter");

	ImGui::Checkbox("Add default bone", &import_options.add_bone);
	ImGui::Text("Import model type:");
	ImGui::RadioButton("Animated", &import_options.vertex_type, loaded_model::animated);
	ImGui::RadioButton("Static", &import_options.vertex_type, loaded_model::static_object);

	if (ImGui::Button("Load OBJ or COLLADA file")) {
		std::vector<no::model_data<no::animated_mesh_vertex>> models;
		std::string browsed_path = no::platform::open_file_browse_window();
		no::model_conversion_options options;
		options.import.bones = { import_options.add_bone, model.name };
		options.exporter = [&](const std::string& destination, const no::model_data<no::animated_mesh_vertex>& data) {
			models.push_back(data);
		};
		if (import_options.vertex_type == loaded_model::animated) {
			auto path = std::filesystem::path(browsed_path).parent_path();
			auto files = no::entries_in_directory(path.string(), no::entry_inclusion::only_files, false);
			for (auto& file : files) {
				auto extension = no::file_extension_in_path(file);
				if (extension != ".dae" && extension != ".obj") {
					continue;
				}
				std::string name = std::filesystem::path(file).stem().string();
				no::convert_model(file, name, options);
			}
			auto merged_model = no::merge_model_animations<no::animated_mesh_vertex>(models);
			merged_model.name = std::filesystem::path(browsed_path).parent_path().stem().string();
			if (merged_model.animations.empty()) {
				auto& default_animation = merged_model.animations.emplace_back();
				default_animation.name = std::filesystem::path(files.front()).stem().string();
				default_animation.channels.insert(default_animation.channels.begin(), merged_model.nodes.size(), {});
			}
			model.load(merged_model.name, import_options.vertex_type, merged_model);
		} else if (import_options.vertex_type == loaded_model::static_object) {
			no::convert_model(browsed_path, model.name, options);
			if (!models.empty()) {
				models[0].texture = model.data.texture;
				model.load(models[0].name, import_options.vertex_type, models[0]);
			}
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
	imgui_input_text<128>("##TextureName", model.data.texture);
	ImGui::SameLine();
	if (ImGui::Button("Apply##ApplyTexture")) {
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

	int old_animation = model.animation_index;
	ImGui::ListBox("##SelectAnimation", &model.animation_index, [](void* data, int i, const char** out) -> bool {
		auto& model = *(no::model*)data;
		if (i < 0 || i >= model.total_animations()) {
			return false;
		}
		*out = model.animation(i).name.c_str();
		return true;
	}, &model.model, model.model.total_animations(), 10);
	if (old_animation != model.animation_index) {
		model.animator.play(0, model.animation_index, -1);
	}

	ImGui::Separator();

	ImGui::Text("Export as ");
	ImGui::SameLine();
	imgui_input_text<128>("##ExportModelName", model.name);
	if (ImGui::Button("Export NOM model")) {
		std::string path = no::asset_path("models/" + model.name + ".nom");
		if (!overwrite_export) {
			while (std::filesystem::exists(path)) {
				path.insert(path.end() - 4, '_');
			}
		}
		if (model.vertex_type == loaded_model::animated) {
			no::export_model(path, model.data);
		} else if (model.vertex_type == loaded_model::static_object) {
			no::export_model(path, model.data.to<static_object_vertex>([](const no::animated_mesh_vertex& vertex) {
				return static_object_vertex{ vertex.position, vertex.normal, vertex.tex_coords };
			}));
		}
	}
	ImGui::SameLine();
	ImGui::Checkbox("Overwrite file", &overwrite_export);
}

void converter_tool::draw() {
	no::set_shader_model(transform);
	model.draw();
}

attachments_tool::attachments_tool() : animator{ root_model } {
	animator.add();
	mappings.load(no::asset_path("models/attachments.noma"));
	active_attachments.reserve(100); // todo: make this unnecessary. it isn't critical either way
}

attachments_tool::~attachments_tool() {
	no::delete_texture(texture);
	for (auto& attachment : active_attachments) {
		no::delete_texture(attachment.texture);
	}
}

void attachments_tool::update() {
	ImGui::Text("Attachments");
	if (!root_model.is_drawable()) {
		std::string path;
		if (ImGui::Button("Import main NOM model")) {
			path = no::platform::open_file_browse_window();
		}
		if (ImGui::Button("Import character")) {
			path = no::asset_path("models/character.nom");
		}
		if (!path.empty()) {
			root_model.load<no::animated_mesh_vertex>(path);
			no::delete_texture(texture);
			texture = no::create_texture({ no::asset_path("textures/" + root_model.texture_name() + ".png") });
		}
		return;
	} else {
		ImGui::Text(CSTRING("Managing attachments for: " << root_model.name()));
	}
	ImGui::Separator();
	ImGui::Checkbox("Freeze animation", &freeze_animation);
	ImGui::Text("View animation");
	int old_animation = animation;
	ImGui::ListBox("##ViewAnimationList", &animation, [](void* data, int i, const char** out) -> bool {
		auto& model = *(no::model*)data;
		if (i < 0 || i >= model.total_animations()) {
			return false;
		}
		*out = model.animation(i).name.data();
		return true;
	}, &root_model, root_model.total_animations(), 15);
	animator.play(0, animation, -1);
	animator.sync();

	ImGui::Separator();

	if (root_model.total_animations() > 0 && ImGui::CollapsingHeader(CSTRING("Create new mapping for " << root_model.animation(animation).name))) {
		ImGui::Text("Attachment model");
		ImGui::InputText("##NewMappingModel", temp_mapping.model, 100);
		ImGui::Text("Attachment animation");
		ImGui::InputText("##NewMappingAnimation", temp_mapping.animation, 100);
		std::string root_animation = mappings.find_root_animation(root_model.name(), temp_mapping.model, temp_mapping.animation);
		if (root_animation != "") {
			ImGui::Checkbox("Reuse default mapping", &reuse_default_mapping);
		} else {
			reuse_default_mapping = false;
		}
		if (!reuse_default_mapping) {
			ImGui::Text("Attach to");
			ImGui::SameLine();
			if (ImGui::BeginCombo("##NewMappingAttachTo", CSTRING(temp_mapping.channel))) {
				for (int i = 0; i < root_model.total_nodes(); i++) {
					if (ImGui::Selectable(CSTRING(i << ": " << root_model.node(i).name))) {
						temp_mapping.channel = i;
					}
				}
				ImGui::EndCombo();
			}
		}
		no::bone_attachment_mapping new_mapping;
		new_mapping.root_model = root_model.name();
		new_mapping.root_animation = root_model.animation(animation).name;
		new_mapping.attached_model = temp_mapping.model;
		new_mapping.attached_animation = temp_mapping.animation;
		bool already_exists = mappings.exists(new_mapping);
		if (already_exists) {
			ImGui::Text("This mapping already exists.");
		}
		if (!already_exists && ImGui::Button("Save##SaveNewMapping")) {
			if (reuse_default_mapping) {
				mappings.for_each([&](no::bone_attachment_mapping& mapping) {
					if (mapping.is_same_mapping(new_mapping)) {
						new_mapping.attached_to_channel = mapping.attached_to_channel;
						new_mapping.position = mapping.position;
						new_mapping.rotation = mapping.rotation;
						return false;
					}
					return true;
				});
			} else {
				new_mapping.attached_to_channel = temp_mapping.channel;
			}
			current_mapping = nullptr;
			mappings.add(new_mapping);
			temp_mapping = {};
		}
		ImGui::Separator();
	}

	ImGui::Text("Existing mappings");
	if (ImGui::BeginCombo("Selected mapping", current_mapping ? current_mapping->mapping_string().c_str() : "None")) {
		if (root_model.total_animations() > animation) {
			mappings.for_each([&](no::bone_attachment_mapping& mapping) {
				if (mapping.root_animation != root_model.animation(animation).name) {
					return true;
				}
				if (ImGui::Selectable(mapping.mapping_string().c_str())) {
					current_mapping = &mapping;
					apply_changes_to_other_animations = false;
				}
				return true;
			});
		}
		ImGui::EndCombo();
	}

	ImGui::Separator();

	if (current_mapping) {
		ImGui::Text("Selected mapping:");
		ImGui::Text(CSTRING(current_mapping->attached_model << " is attached to"));
		ImGui::SameLine();
		std::string node_name = STRING(current_mapping->attached_to_channel << ": " << root_model.node(current_mapping->attached_to_channel).name);
		if (ImGui::BeginCombo("##SelectChannelCombo", node_name.c_str())) {
			for (int i = 0; i < root_model.total_nodes(); i++) {
				if (ImGui::Selectable(CSTRING(i << ": " << root_model.node(i).name))) {
					current_mapping->attached_to_channel = i;
				}
			}
			ImGui::EndCombo();
		}
		ImGui::Text("Position");
		ImGui::SameLine();
		ImGui::InputFloat3("##AttachmentPosition", &current_mapping->position.x, 5);
		glm::vec4 right = glm::inverse(glm::mat4_cast(current_mapping->rotation)) * glm::vec4(1.0f, 0.0f, 0.0f, 1.0f) * 0.05f;
		glm::vec4 forward = glm::inverse(glm::mat4_cast(current_mapping->rotation)) * glm::vec4(0.0f, 0.0f, -1.0f, 1.0f) * 0.05f;
		glm::vec4 up = glm::inverse(glm::mat4_cast(current_mapping->rotation)) * glm::vec4(0.0f, 1.0f, 0.0f, 1.0f) * 0.05f;
		if (ImGui::Button("Left##PositionLeft")) {
			current_mapping->position -= { right.x, right.y, right.z };
		}
		ImGui::SameLine();
		if (ImGui::Button("Right##PositionRight")) {
			current_mapping->position += { right.x, right.y, right.z };
		}
		ImGui::SameLine();
		if (ImGui::Button("Up##PositionUp")) {
			current_mapping->position += { up.x, up.y, up.z };
		}
		ImGui::SameLine();
		if (ImGui::Button("Down##PositionDown")) {
			current_mapping->position -= { up.x, up.y, up.z };
		}
		ImGui::SameLine();
		if (ImGui::Button("Forward##PositionForward")) {
			current_mapping->position += { forward.x, forward.y, forward.z };
		}
		ImGui::SameLine();
		if (ImGui::Button("Backwards##PositionBackwards")) {
			current_mapping->position -= { forward.x, forward.y, forward.z };
		}

		ImGui::Text("Rotation");
		ImGui::SameLine();
		float wxyz[4] = {
			current_mapping->rotation.w,
			current_mapping->rotation.x,
			current_mapping->rotation.y,
			current_mapping->rotation.z
		};
		ImGui::InputFloat4("##AttachmentRotation", wxyz, 5);
		current_mapping->rotation = { wxyz[0], wxyz[1], wxyz[2], wxyz[3] };

		if (ImGui::Button("+##RotateXPlus")) {
			current_mapping->rotation = glm::rotate(current_mapping->rotation, 0.01f, { 1.0f, 0.0f, 0.0f });
		}
		ImGui::SameLine();
		ImGui::Text("X");
		ImGui::SameLine();
		if (ImGui::Button("-##RotateXMinus")) {
			current_mapping->rotation = glm::rotate(current_mapping->rotation, 0.01f, { -1.0f, 0.0f, 0.0f });
		}
		ImGui::SameLine();
		if (ImGui::Button("+##RotateYPlus")) {
			current_mapping->rotation = glm::rotate(current_mapping->rotation, 0.01f, { 0.0f, 1.0f, 0.0f });
		}
		ImGui::SameLine();
		ImGui::Text("Y");
		ImGui::SameLine();
		if (ImGui::Button("-##RotateYMinus")) {
			current_mapping->rotation = glm::rotate(current_mapping->rotation, 0.01f, { 0.0f, -1.0f, 0.0f });
		}
		ImGui::SameLine();
		if (ImGui::Button("+##RotateZPlus")) {
			current_mapping->rotation = glm::rotate(current_mapping->rotation, 0.01f, { 0.0f, 0.0f, 1.0f });
		}
		ImGui::SameLine();
		ImGui::Text("Z");
		ImGui::SameLine();
		if (ImGui::Button("-##RotateZMinus")) {
			current_mapping->rotation = glm::rotate(current_mapping->rotation, 0.01f, { 0.0f, 0.0f, -1.0f });
		}

		ImGui::Checkbox("Apply changes to other animations", &apply_changes_to_other_animations);
		if (apply_changes_to_other_animations) {
			mappings.for_each([&](no::bone_attachment_mapping& mapping) {
				if (mapping.is_same_mapping(*current_mapping)) {
					mapping.position = current_mapping->position;
					mapping.rotation = current_mapping->rotation;
					mapping.attached_to_channel = current_mapping->attached_to_channel;
				}
				return true;
			});
		}
		ImGui::Separator();
	}

	ImGui::Text("Loaded attachments");
	for (int i = 0; i < (int)active_attachments.size(); i++) {
		auto& attachment = active_attachments[i];
		if (!attachment.alive) {
			continue;
		}
		bool exists = false;
		if (attachment.model.index_of_animation("default") != -1) {
			mappings.for_each([&](no::bone_attachment_mapping& mapping) {
				if (mapping.attached_model == attachment.model.name()) {
					for (int i = 0; i < root_model.total_animations(); i++) {
						if (root_model.animation(i).name == mapping.root_animation) {
							exists = true;
							break;
						}
					}
				}
				return !exists;
			});
		} else {
			exists = true;
		}
		if (attachment.animator->count() == 0) {
			if (!exists) {
				ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
				ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
			}
			if (ImGui::Button(CSTRING("Attach##AttachToModel" << i))) {
				attachment.animator->add();
			}
			if (!exists) {
				ImGui::PopStyleVar();
				ImGui::PopItemFlag();
			}
			ImGui::SameLine();
		} else if (exists) {
			if (attachment.model.index_of_animation("default") != -1) {
				attachment.animator->play(0, "default", -1);
				attachment.animator->set_is_attachment(0, true);
				auto bone = attachment.animator->get_attachment_bone(0);
				mappings.update(root_model, animation, attachment.model.name(), bone);
				attachment.animator->set_attachment_bone(0, bone);
				attachment.animator->set_root_transform(0, animator.get_transform(0, bone.parent));
			} else {
				attachment.animator->play(0, animation, -1);
			}
			if (ImGui::Button(CSTRING("Detach##DetachFromModel" << i))) {
				attachment.animator->erase(0);
			}
			ImGui::SameLine();
		}
		if (attachment.animator->count() > 0) {
			ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
		}
		if (ImGui::Button(CSTRING("Remove##RemoveAttachmentModel" << i))) {
			active_attachments[i].alive = false;
			continue;
		}
		if (attachment.animator->count() > 0) {
			ImGui::PopStyleVar();
			ImGui::PopItemFlag();
		}
		ImGui::SameLine();
		ImGui::Text(attachment.model.name().c_str());
	}

	ImGui::Separator();

	if (root_model.is_drawable()) {
		std::string path;
		if (ImGui::Button("Load attachment")) {
			path = no::platform::open_file_browse_window();
		}
		ImGui::SameLine();
		if (ImGui::Button("[Shield]")) {
			path = no::asset_path("models/shield.nom");
		}
		ImGui::SameLine();
		if (ImGui::Button("[Sword]")) {
			path = no::asset_path("models/sword.nom");
		}
		ImGui::SameLine();
		if (ImGui::Button("[Axe]")) {
			path = no::asset_path("models/axe.nom");
		}
		ImGui::SameLine();
		if (ImGui::Button("[Spear]")) {
			path = no::asset_path("models/spear.nom");
		}
		if (ImGui::Button("[Pants]")) {
			path = no::asset_path("models/pants.nom");
		}
		ImGui::SameLine();
		if (ImGui::Button("[Shirt]")) {
			path = no::asset_path("models/shirt.nom");
		}
		ImGui::SameLine();
		if (ImGui::Button("[Helm]")) {
			path = no::asset_path("models/helm.nom");
		}
		ImGui::SameLine();
		if (ImGui::Button("[Shoes]")) {
			path = no::asset_path("models/shoes.nom");
		}
		if (!path.empty()) {
			bool found = false;
			for (auto& attachment : active_attachments) {
				if (!attachment.alive) {
					attachment.animator->erase(0);
					attachment.model.load<no::animated_mesh_vertex>(path);
					no::delete_texture(attachment.texture);
					attachment.texture = no::create_texture({ no::asset_path("textures/" + attachment.model.texture_name() + ".png") });
					attachment.alive = true;
					found = true;
					break;
				}
			}
			if (!found) {
				auto& attachment = active_attachments.emplace_back();
				attachment.model.load<no::animated_mesh_vertex>(path);
				attachment.texture = no::create_texture({ no::asset_path("textures/" + attachment.model.texture_name() + ".png") });
			}
		}
	}

	ImGui::Separator();

	if (ImGui::Button("Save attachment mappings")) {
		mappings.save(no::asset_path("models/attachments.noma"));
	}
}

void attachments_tool::draw() {
	if (!animator.can_animate(0)) {
		return;
	}
	no::set_shader_model(no::transform3{});
	if (freeze_animation) {
		animator.reset(0);
	}
	animator.animate();
	if (texture != -1) {
		no::bind_texture(texture);
	}
	animator.shader.bones = no::get_shader_variable("uni_Bones");
	animator.draw();
	for (auto& attachment : active_attachments) {
		attachment.animator->sync();
		if (attachment.alive && attachment.animator->can_animate(0)) {
			if (attachment.texture != -1) {
				no::bind_texture(attachment.texture);
			}
			attachment.animator->shader.bones = no::get_shader_variable("uni_Bones");
			attachment.animator->animate();
			attachment.animator->draw();
		}
	}
}

model_manager_state::model_manager_state() : dragger(mouse()) {
	no::imgui::create(window());
	mouse_scroll_id = mouse().scroll.listen([this](int steps) {
		if (is_mouse_over_ui()) {
			return;
		}
		float& factor = camera.rotation_offset_factor;
		factor -= (float)steps;
		if (factor > 12.0f) {
			factor = 12.0f;
		} else if (factor < 4.0f) {
			factor = 4.0f;
		}
	});
	camera.transform.position.y = 1.0f;
	camera.rotation_offset_factor = 12.0f;
	animate_shader = no::create_shader(no::asset_path("shaders/animatediffuse"));
	static_shader = no::create_shader(no::asset_path("shaders/staticdiffuse"));
}

model_manager_state::~model_manager_state() {
	mouse().scroll.ignore(mouse_scroll_id);
	no::delete_shader(animate_shader);
	no::delete_shader(static_shader);
	no::imgui::destroy();
}

void model_manager_state::update() {
	camera.size = window().size().to<float>();
	no::imgui::start_frame();
	menu_bar_state::update();
	ImGuiWindowFlags imgui_flags
		= ImGuiWindowFlags_NoMove
		| ImGuiWindowFlags_NoResize
		| ImGuiWindowFlags_NoCollapse
		| ImGuiWindowFlags_NoTitleBar;
	ImGui::SetNextWindowPos({ 0.0f, 20.0f }, ImGuiSetCond_Once);
	ImGui::SetNextWindowSize({ 480.0f, (float)window().height() - 20.0f }, ImGuiSetCond_Always);
	ImGui::Begin("##ModelManager", nullptr, imgui_flags);
	ImGui::Text(CSTRING("FPS: " << frame_counter().current_fps()));
	ImGui::Separator();

	ImGui::Text("Current tool:");
	ImGui::SameLine();
	ImGui::RadioButton("Converter", &current_tool, 0);
	ImGui::SameLine();
	ImGui::RadioButton("Attachments", &current_tool, 1);
	ImGui::Separator();

	if (current_tool == 0) {
		converter.update();
	} else if (current_tool == 1) {
		attachments.update();
	}
	if (!is_mouse_over_ui()) {
		dragger.update(camera);
	}
	camera.update();

	ImGui::Separator();

	ImGui::End();
	no::imgui::end_frame();
}

void model_manager_state::draw() {
	if (converter.model.type() == 0) {
		no::bind_shader(animate_shader);
	} else {
		no::bind_shader(static_shader);
	}
	no::set_shader_view_projection(camera);
	no::get_shader_variable("uni_LightPosition").set(camera.transform.position + camera.offset());
	no::get_shader_variable("uni_LightColor").set(no::vector3f{ 1.0f, 1.0f, 1.0f });
	no::get_shader_variable("uni_Color").set(no::vector4f{ 1.0f });
	if (current_tool == 0) {
		converter.draw();
	} else if (current_tool == 1) {
		attachments.draw();
	}
	no::imgui::draw();
}

bool model_manager_state::is_mouse_over_ui() const {
	return mouse().position().x < 480;
}
