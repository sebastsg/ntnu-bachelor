#pragma once

#include "loop.hpp"
#include "camera.hpp"
#include "draw.hpp"

class loaded_model {
public:

	std::string name;

	loaded_model(const std::string& name, int vertex_type, const no::model_data<no::animated_mesh_vertex>& data);

	loaded_model(loaded_model&&);
	loaded_model& operator=(loaded_model&&);

	void draw();

	int type() const {
		return vertex_type;
	}

private:

	no::model model;
	no::model_instance instance;
	no::model_data<no::animated_mesh_vertex> data;
	int vertex_type = 0;
	int texture = -1;

};

class converter_tool {
public:


private:

	no::transform transform;
	loaded_model model;

};

class model_manager_state : public no::window_state {
public:

	model_manager_state();
	~model_manager_state() override;

	void update() override;
	void draw() override;

private:

	no::perspective_camera camera;
	no::perspective_camera::drag_controller dragger;

	no::transform model_transform;
	std::vector<loaded_model> models;
	int active_model = 0;

	int animate_shader = -1;
	int static_shader = -1;

	struct {
		bool add_bone = false;
		char name[64] = {};
		int vertex_type = 0;
	} import_options;

	int mouse_scroll_id = -1;

};
