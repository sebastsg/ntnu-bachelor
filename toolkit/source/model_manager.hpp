#pragma once

#include "menu.hpp"
#include "camera.hpp"
#include "skeletal.hpp"

class converter_tool;

class loaded_model {
public:

	friend class converter_tool;

	static constexpr int animated = 0;
	static constexpr int static_object = 1;

	loaded_model() : animator{ model } {}
	loaded_model(const loaded_model&) = delete;
	loaded_model(loaded_model&&) = delete;

	loaded_model& operator=(const loaded_model&) = delete;
	loaded_model& operator=(loaded_model&&) = delete;

	void draw();
	int type() const;
	void scale_vertices(no::vector3f scale);
	void load_texture();
	void load(const std::string& name, int vertex_type, const no::model_data<no::animated_mesh_vertex>& data);

private:

	std::string name;
	no::model_data<no::animated_mesh_vertex> data;
	no::model model;
	no::skeletal_animator animator;
	int animation_index = 0;
	int vertex_type = 0;
	int texture = -1;

};

class converter_tool {
public:

	no::transform3 transform;
	loaded_model model;
	std::vector<std::string> node_names_for_list;

	converter_tool();

	void update();
	void draw();

private:

	struct {
		bool add_bone = true;
		int vertex_type = 0;
	} import_options;

	bool overwrite_export = false;
	int current_node = 0;

};

class attachments_tool {
public:

	attachments_tool();
	~attachments_tool();

	void update();
	void draw();

private:

	no::model root_model;

	no::bone_attachment_mapping_list mappings;
	no::bone_attachment_mapping* current_mapping = nullptr;

	struct {
		char model[100] = {};
		char animation[100] = { "default" };
		int channel = -1;
	} temp_mapping;

	bool reuse_default_mapping = false;
	bool apply_changes_to_other_animations = false;

	struct active_attachment_data {
		bool alive = true;
		no::model model;
		std::unique_ptr<no::skeletal_animator> animator;
		int texture = -1;
		active_attachment_data() : animator{ std::make_unique<no::skeletal_animator>(model) } {
			
		}
	};

	std::vector<active_attachment_data> active_attachments;
	no::skeletal_animator animator;
	
	int animation = 0;
	int texture = -1;
	bool freeze_animation = false;

};

class model_manager_state : public menu_bar_state {
public:

	model_manager_state();
	~model_manager_state() override;

	void update() override;
	void draw() override;

	bool is_mouse_over_ui() const;

private:

	int current_tool = 0;
	int mouse_scroll_id = -1;

	int animate_shader = -1;
	int static_shader = -1;

	converter_tool converter;
	attachments_tool attachments;

	no::perspective_camera camera;
	no::perspective_camera::drag_controller dragger;

};
