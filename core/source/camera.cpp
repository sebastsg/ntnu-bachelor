#include "camera.hpp"
#include "input.hpp"

namespace no {

ortho_camera::ortho_camera() {
	transform.position.z = 1.0f;
}

void ortho_camera::update() {
	if (target) {
		vector2f goal = {
			(target->position.x + target->scale.x / 2.0f) - width() / target_chase_aspect.x,
			(target->position.y + target->scale.y / 2.0f) - height() / target_chase_aspect.y
		};
		vector2f delta = {
			(goal.x - x()) * target_chase_speed.x,
			(goal.y - y()) * target_chase_speed.y
		};
		if (std::abs(delta.x) > 0.4f) {
			transform.position.x += delta.x;
		}
		if (std::abs(delta.y) > 0.4f) {
			transform.position.y += delta.y;
		}
	}
	transform.rotation.z = fmodf(transform.rotation.z, 360.0f);
	if (transform.rotation.z < 0.0f) {
		transform.rotation.z += 360.0f;
	}
}

float ortho_camera::x() const {
	return transform.position.x / zoom;
}

float ortho_camera::y() const {
	return transform.position.y / zoom;
}

vector2f ortho_camera::position() const {
	return transform.position.xy / zoom;
}

float ortho_camera::width() const {
	return transform.scale.x / zoom;
}

float ortho_camera::height() const {
	return transform.scale.y / zoom;
}

vector2f ortho_camera::size() const {
	return transform.scale.xy / zoom;
}

vector2f ortho_camera::mouse_position(const mouse& mouse) const {
	return (transform.position.xy + mouse.position().to<float>()) / zoom;
}

glm::mat4 ortho_camera::rotation() const {
	return glm::rotate(glm::mat4(1.0f), transform.rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
}

glm::mat4 ortho_camera::projection() const {
	return glm::ortho(0.0f, transform.scale.x, transform.scale.y, 0.0f, near_clipping_plane, far_clipping_plane);
}

glm::mat4 ortho_camera::view() const {
	vector3f rounded_position = transform.position;
	rounded_position.xy.ceil();
	glm::vec3 negated_position = {
		-rounded_position.x,
		-rounded_position.y,
		-rounded_position.z
	};
	glm::mat4 matrix = rotation() * glm::translate(glm::mat4(1.0f), negated_position);
	return glm::scale(matrix, { zoom, zoom, zoom });
}

glm::mat4 ortho_camera::view_projection() const {
	return view() * projection();
}

void perspective_camera::update() {
	aspect_ratio = size.x / size.y;
	update_rotation();
}

glm::mat4 perspective_camera::translation() const {
	glm::vec3 position = { -transform.position.x, -transform.position.y, -transform.position.z };
	position -= glm::vec3{ rotation_offset.x, rotation_offset.y, rotation_offset.z };
	return glm::translate(glm::mat4(1.0f), position);
}

glm::mat4 perspective_camera::rotation() const {
	auto matrix = glm::rotate(glm::mat4(1.0f), deg_to_rad(transform.rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
	matrix = glm::rotate(matrix, deg_to_rad(transform.rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
	return glm::rotate(matrix, deg_to_rad(transform.rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
}

glm::mat4 perspective_camera::projection() const {
	return glm::perspective(deg_to_rad(field_of_view), aspect_ratio, near_clipping_plane, far_clipping_plane);
}

glm::mat4 perspective_camera::view() const {
	auto matrix = rotation() * translation();
	return glm::scale(matrix, { transform.scale.x, transform.scale.y, transform.scale.z });
}

glm::mat4 perspective_camera::view_projection() const {
	return view() * projection();
}

vector3f perspective_camera::forward() const {
	const auto result = glm::inverse(rotation()) * glm::vec4(0.0f, 0.0f, -1.0f, 1.0f);
	return { result.x, result.y, result.z };
}

vector3f perspective_camera::right() const {
	const auto result = glm::inverse(rotation()) * glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
	return { result.x, result.y, result.z };
}

vector3f perspective_camera::up() const {
	const auto result = glm::inverse(rotation()) * glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
	return { result.x, result.y, result.z };
}

vector3f perspective_camera::offset() const {
	return rotation_offset;
}

ray perspective_camera::unproject(const vector2f& position_in_window) const {
	glm::vec4 viewport = { 0.0f, 0.0f, size.x, size.y };
	glm::vec3 window_near = { position_in_window.x, size.y - position_in_window.y, 0.0f };
	glm::vec3 window_far = { position_in_window.x, size.y - position_in_window.y, 1.0f };
	glm::vec3 world_near = glm::unProject(window_near, view(), projection(), viewport);
	glm::vec3 world_far = glm::unProject(window_far, view(), projection(), viewport);
	glm::vec3 origin = world_near;
	glm::vec3 direction = glm::normalize(world_far);
	return { { origin.x, origin.y, origin.z }, { direction.x, direction.y, direction.z } };
}

ray perspective_camera::unproject(const mouse& mouse) const {
	return unproject(mouse.position().to<float>());
}

void perspective_camera::update_rotation() {
	transform.rotation.x = std::fmodf(transform.rotation.x, 360.0f);
	transform.rotation.y = std::fmodf(transform.rotation.y, 360.0f);
	if (transform.rotation.x < 5.0f) {
		transform.rotation.x = 5.0f;
	} else if (transform.rotation.x > 89.0f) {
		transform.rotation.x = 89.0f;
	}
	if (transform.rotation.y > 180.0f) {
		transform.rotation.y -= 360.0f;
	} else if (transform.rotation.y <= -180.0f) {
		transform.rotation.y += 360.0f;
	}
	const float rad_x = deg_to_rad(transform.rotation.x);
	const float rad_y = deg_to_rad(transform.rotation.y);
	const float cos_x = std::cos(rad_x);
	const float cos_y = std::cos(rad_y);
	const float sin_x = std::sin(rad_x);
	const float sin_y = std::sin(rad_y);
	rotation_offset.x = -cos_x * sin_y * rotation_offset_factor;
	rotation_offset.y = sin_x * rotation_offset_factor;
	rotation_offset.z = cos_x * cos_y * rotation_offset_factor;
}

perspective_camera::drag_controller::drag_controller(mouse& mouse) : mouse_(mouse) {
	press_id = mouse.press.listen([this](const mouse::press_message& event) {
		last_mouse_position = mouse_.position().to<float>();
	});
}

perspective_camera::drag_controller::~drag_controller() {
	mouse_.press.ignore(press_id);
}

void perspective_camera::drag_controller::update(perspective_camera& camera) {
	if (mouse_.is_button_down(mouse::button::middle)) {
		vector2f mouse_position = mouse_.position().to<float>();
		vector2f delta = mouse_position - last_mouse_position;
		last_mouse_position = mouse_position;
		camera.transform.rotation.y += delta.x * speed.x;
		camera.transform.rotation.x += delta.y * speed.y;
	}
}

void perspective_camera::rotate_controller::update(perspective_camera& camera, const keyboard& keyboard) const {
	if (keyboard.is_key_down(up)) {
		camera.transform.rotation.x += speed.y;
	}
	if (keyboard.is_key_down(left)) {
		camera.transform.rotation.y += speed.x;
	}
	if (keyboard.is_key_down(down)) {
		camera.transform.rotation.x -= speed.y;
	}
	if (keyboard.is_key_down(right)) {
		camera.transform.rotation.y -= speed.x;
	}
}

void perspective_camera::move_controller::update(perspective_camera& camera, const keyboard& keyboard) const {
	if (keyboard.is_key_down(forward)) {
		camera.transform.position += camera.forward();
	}
	if (keyboard.is_key_down(left)) {
		camera.transform.position -= camera.right();
	}
	if (keyboard.is_key_down(backward)) {
		camera.transform.position -= camera.forward();
	}
	if (keyboard.is_key_down(right)) {
		camera.transform.position += camera.right();
	}
	if (keyboard.is_key_down(up)) {
		camera.transform.position += camera.up();
	}
	if (keyboard.is_key_down(down)) {
		camera.transform.position -= camera.up();
	}
}

void perspective_camera::follow_controller::update(perspective_camera& camera, const no::transform& transform) const {
	// todo: interpolate
	camera.transform.position = transform.position + offset;
}

}
