#pragma once

#include "loop.hpp"
#include "camera.hpp"
#include "draw.hpp"

class model_manager_state : public no::window_state {
public:

	model_manager_state();
	~model_manager_state() override;

	void update() override;
	void draw() override;

private:

	no::perspective_camera camera;

	no::model model;
	no::model_instance instance;
	no::model_data<no::animated_mesh_vertex> model_data;
	int model_texture = -1;

	int shader = -1;

};
