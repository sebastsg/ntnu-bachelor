#pragma once

#include "platform.hpp"

#if ENABLE_GRAPHICS

#include "transform.hpp"
#include "input.hpp"

namespace no {

class ortho_camera {
public:

	transform2* target = nullptr;
	transform2 transform;
	float zoom = 1.0f;

	float near_clipping_plane = 0.01f;
	float far_clipping_plane = 1000.0f;

	vector2f target_chase_speed = { 0.05f, 0.05f };
	vector2f target_chase_aspect = { 2.0f, 1.5f };

	void update();

	// from screen to world coordinates (divided by zoom)
	float x() const;
	float y() const;
	vector2f position() const;
	float width() const;
	float height() const;
	vector2f size() const;
	vector2f mouse_position(const mouse& mouse) const;

	glm::mat4 rotation() const;
	glm::mat4 view() const;
	glm::mat4 projection() const;
	glm::mat4 view_projection() const;

};

class perspective_camera {
public:

	class drag_controller;
	class rotate_controller;
	class move_controller;
	class follow_controller;

	transform3 transform;
	vector2f size;

	float field_of_view = 60.0f;
	float near_clipping_plane = 0.01f;
	float far_clipping_plane = 150.0f;

	float rotation_offset_factor = 8.0f;

	void update();

	glm::mat4 translation() const;
	glm::mat4 rotation() const;
	glm::mat4 view() const;
	glm::mat4 projection() const;
	glm::mat4 view_projection() const;

	vector3f forward() const;
	vector3f right() const;
	vector3f up() const;
	
	vector3f offset() const;

	ray unproject(const vector2f& position_in_window) const;
	ray unproject(const mouse& mouse) const;

	vector2f world_to_screen(const vector3f& position) const;

private:
	
	void update_rotation();

	float aspect_ratio = 1.33f;
	vector3f rotation_offset;

};

// todo: if gamepad support is added, these controllers should support that interface
//       instead of using key/keyboard/mouse directly. unsure about drag.

class perspective_camera::drag_controller {
public:

	vector2f speed = 0.2f;

	drag_controller(mouse& mouse);
	~drag_controller();

	void update(perspective_camera& camera);

private:

	mouse& mouse_;
	vector2f last_mouse_position;
	int press_id = -1;

};

class perspective_camera::rotate_controller {
public:

	vector2f speed = 2.0f;

	key up = key::up;
	key left = key::left;
	key down = key::down;
	key right = key::right;

	void update(perspective_camera& camera, const keyboard& keyboard) const;

};

class perspective_camera::move_controller {
public:

	vector2f speed = 2.0f;

	key forward = key::w;
	key left = key::a;
	key backward = key::s;
	key right = key::d;
	key up = key::r;
	key down = key::f;

	void update(perspective_camera& camera, const keyboard& keyboard) const;

};

class perspective_camera::follow_controller {
public:

	vector3f speed = 2.0f;
	vector3f offset = { 0.0f, 1.0f, 0.0f };

	void update(perspective_camera& camera, const transform3& transform) const;

};

}

#endif
