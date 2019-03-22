#include "object_editor.hpp"
#include "window.hpp"
#include "assets.hpp"
#include "platform.hpp"
#include "surface.hpp"
#include "dialogue.hpp"

#include "imgui/imgui.h"
#include "imgui/imgui_platform.h"

object_editor::object_editor() : dragger(mouse()), animator{ model } {
	window().set_clear_color({ 0.3f });
	no::imgui::create(window());
	box.load(no::create_box_model_data<no::static_mesh_vertex>([](const no::vector3f& vertex) {
		return no::static_mesh_vertex{ vertex, 1.0f };
	}));
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
	animator.add();
	camera.transform.position.y = 1.0f;
	camera.rotation_offset_factor = 12.0f;
	animate_shader = no::create_shader(no::asset_path("shaders/animatediffuse"));
	static_shader = no::create_shader(no::asset_path("shaders/staticdiffuse"));
	blank_texture = no::create_texture({ 2, 2, no::pixel_format::rgba, 0xFFBBAAFF });
}

object_editor::~object_editor() {
	mouse().scroll.ignore(mouse_scroll_id);
	no::delete_shader(animate_shader);
	no::delete_shader(static_shader);
	no::delete_texture(blank_texture);
	no::delete_texture(model_texture);
	no::imgui::destroy();
}

void object_editor::object_type_combo(game_object_type& target, const std::string& ui_id) {
	if (ImGui::BeginCombo(CSTRING("Object type##ObjectType" << ui_id), CSTRING(target))) {
		for (int type_value = 0; type_value < (int)game_object_type::total_types; type_value++) {
			game_object_type type = (game_object_type)type_value;
			if (ImGui::Selectable(CSTRING(type << "##ObjectType" << ui_id << type_value))) {
				target = type;
			}
		}
		ImGui::End();
	}
}

void object_editor::ui_create_object() {
	if (ImGui::CollapsingHeader("Create new object")) {
		ImGui::InputText("Name##NewObjectName", new_object.name, 100);
		object_type_combo(new_object.type, "New");
		ImGui::InputText("Model##NewObjectModel", new_object.model, 100);
		if (ImGui::Button("Save##SaveNewObject")) {
			game_object_definition object;
			object.id = object_definitions().count();
			object.type = new_object.type;
			object.name = new_object.name;
			object.model = new_object.model;
			object_definitions().add(object);
			new_object = {};
		}
	}
}

void object_editor::ui_select_object() {
	ImGui::Text("Select object:");
	ImGui::SameLine();
	int previous_object = current_object;
	if (ImGui::BeginCombo("##SelectEditObject", object_definitions().get(current_object).name.c_str())) {
		for (int id = 0; id < object_definitions().count(); id++) {
			if (ImGui::Selectable(object_definitions().get(id).name.c_str())) {
				current_object = id;
			}
		}
		ImGui::EndCombo();
	}
	auto& selected = object_definitions().get(current_object);
	if (selected.id == -1) {
		return;
	}
	if (previous_object != current_object) {
		if (selected.type == game_object_type::character) {
			model.load<no::animated_mesh_vertex>(no::asset_path("models/" + selected.model + ".nom"));
			if (selected.model == "character") {
				// todo: select animation list, like in model manager
				animator.play(0, model.index_of_animation("idle"), -1);
			}
		} else {
			model.load<no::static_mesh_vertex>(no::asset_path("models/" + selected.model + ".nom"));
		}
		no::delete_texture(model_texture);
		model_texture = no::create_texture(no::surface(no::asset_path("textures/" + model.texture_name() + ".png")), no::scale_option::nearest_neighbour, true);
	}
	imgui_input_text<128>("Name##EditObjectName", selected.name);
	imgui_input_text<128>("Description##EditObjectDescription", selected.description);
	object_type_combo(selected.type, "Edit");
	imgui_input_text<128>("Model##EditObjectModel", selected.model);
	ImGui::InputInt("Dialogue ID##EditObjectDialogue", &selected.dialogue_id);
	if (!dialogue_meta().find(selected.dialogue_id)) {
		selected.dialogue_id = -1;
	}
	ImGui::Separator();
	ImGui::InputFloat3("Min##EditObjectBBoxMin", &selected.bounding_box.position.x);
	ImGui::InputFloat3("Max##EditObjectBBoxMax", &selected.bounding_box.scale.x);
	if (ImGui::Button("Use model bounding box##EditObjectBBox")) {
		selected.bounding_box = no::load_model_bounding_box(no::asset_path("models/" + selected.model + ".nom"));
	}
	ImGui::InputFloat("Rotate bounding box", &rotation, 45.0f, 45.0f);
	if (rotation >= 360.0f || rotation < 0.0f) {
		rotation = 0.0f;
	}
}

void object_editor::update() {
	camera.size = window().size().to<float>();
	if (!is_mouse_over_ui()) {
		dragger.update(camera);
	}
	camera.update();
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

	ui_create_object();
	ImGui::Separator();
	ui_select_object();

	ImGui::Separator();

	if (ImGui::Button("Save")) {
		object_definitions().save(no::asset_path("objects.data"));
	}

	ImGui::End();
	no::imgui::end_frame();
}

void object_editor::draw() {
	no::imgui::draw();
	auto& selected = object_definitions().get(current_object);
	if (selected.type == game_object_type::character) {
		no::bind_shader(animate_shader);
	} else {
		no::bind_shader(static_shader);
	}
	no::set_shader_view_projection(camera);
	no::get_shader_variable("uni_LightPosition").set(camera.transform.position + camera.offset());
	no::get_shader_variable("uni_LightColor").set(no::vector3f{ 1.0f });
	no::get_shader_variable("uni_FogStart").set(100.0f);
	no::get_shader_variable("uni_FogDistance").set(0.0f);
	if (model_texture != -1) {
		no::bind_texture(model_texture);
	}
	if (selected.type == game_object_type::character && animator.can_animate(0)) {
		no::set_shader_model(no::transform3{ {}, { 0.0f, rotation, 0.0f }, { 1.0f } });
		animator.animate();
		animator.draw();
	} else if (model.is_drawable()) {
		no::draw_shape(model, no::transform3{});
	}
	no::bind_shader(static_shader);
	no::set_shader_view_projection(camera);
	no::bind_texture(blank_texture);
	no::set_polygon_render_mode(no::polygon_render_mode::wireframe);
	no::transform3 box_transform;
	box_transform.position = selected.bounding_box.position;
	box_transform.scale = selected.bounding_box.scale;
	box_transform.rotation.x = 270.0f;
	box_transform.rotation.z = rotation;
	no::draw_shape(box, box_transform.to_matrix4_origin());
	no::set_polygon_render_mode(no::polygon_render_mode::fill);
}

bool object_editor::is_mouse_over_ui() const {
	return mouse().position().x < 320;
}

