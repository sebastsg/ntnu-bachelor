#include "transform.hpp"

namespace no {

float transform::center_x(float width) const {
	return position.x + scale.x / 2.0f - width / 2.0f;
}

float transform::center_y(float height) const {
	return position.y + scale.y / 2.0f - height / 2.0f;
}

float transform::center_z(float depth) const {
	return position.z + scale.z / 2.0f - depth / 2.0f;
}

vector3f transform::center(const vector3f& size) const {
	return {
		position.x + scale.x / 2.0f - size.x / 2.0f,
		position.y + scale.y / 2.0f - size.y / 2.0f,
		position.z + scale.z / 2.0f - size.z / 2.0f
	};
}

void transform::align(align_type alignment, const transform& parent, const vector3f& padding) {
	switch (alignment) {
	case align_type::none:
		break;

	case align_type::top_left:
		position.xy = parent.position.xy;
		position.x += padding.x;
		position.y += padding.y;
		break;

	case align_type::top_middle:
		position.x = parent.center_x(scale.x);
		position.y = parent.position.y;
		position.x -= padding.x / 2.0f;
		position.y += padding.y;
		break;

	case align_type::top_right:
		position.x = parent.position.x + parent.scale.x - scale.x;
		position.y = parent.position.y;
		position.x -= padding.x;
		position.y += padding.y;
		break;

	case align_type::left:
		position.x = parent.position.x;
		position.y = parent.center_y(scale.y);
		position.x += padding.x;
		position.y -= padding.y / 2.0f;
		break;

	case align_type::middle:
		position = parent.center(scale) - padding / 2.0f;
		break;

	case align_type::right:
		position.x = parent.position.x + parent.scale.x - scale.x;
		position.y = parent.center_y(scale.y);
		position.x -= padding.x;
		position.y -= padding.y / 2.0f;
		break;

	case align_type::bottom_left:
		position.x = parent.position.x;
		position.y = parent.position.y + parent.scale.y - scale.y;
		position.x += padding.x;
		position.y -= padding.y;
		break;

	case align_type::bottom_middle:
		position.x = parent.center_x(scale.x);
		position.y = parent.position.y + parent.scale.y - scale.y;
		position.x -= padding.x / 2.0f;
		position.y -= padding.y;
		break;

	case align_type::bottom_right:
		position.x = parent.position.x + parent.scale.x - scale.x;
		position.y = parent.position.y + parent.scale.y - scale.y;
		position.x -= padding.x;
		position.y -= padding.y;
		break;

	default:
		break;
	}
}

bool transform::collides_with(const transform& b) const {
	if (position.x > b.position.x + b.scale.x || position.x + scale.x < b.position.x) {
		return false;
	}
	if (position.y > b.position.y + b.scale.y || position.y + scale.y < b.position.y) {
		return false;
	}
	if (position.z > b.position.z + b.scale.z || position.z + scale.z < b.position.z) {
		return false;
	}
	return true;
}

bool transform::collides_with(const vector3f& b_position, const vector3f& b_scale) const {
	if (position.x > b_position.x + b_scale.x || position.x + scale.x < b_position.x) {
		return false;
	}
	if (position.y > b_position.y + b_scale.y || position.y + scale.y < b_position.y) {
		return false;
	}
	if (position.z > b_position.z + b_scale.z || position.z + scale.z < b_position.z) {
		return false;
	}
	return true;
}

bool transform::collides_with(const vector3f& b_position) const {
	if (b_position.x < position.x || b_position.x > position.x + scale.x) {
		return false;
	}
	if (b_position.y < position.y || b_position.y > position.y + scale.y) {
		return false;
	}
	if (b_position.z < position.z || b_position.z > position.z + scale.z) {
		return false;
	}
	return true;
}

float transform::distance_to(const transform& b) const {
	const float x = (position.x + scale.x / 2.0f - b.position.x + b.scale.x / 2.0f);
	const float y = (position.y + scale.y / 2.0f - b.position.y + b.scale.y / 2.0f);
	const float z = (position.z + scale.z / 2.0f - b.position.z + b.scale.z / 2.0f);
	return std::sqrt(x * x + y * y + z * z);
}

float transform::distance_to(const vector3f& b_position, const vector3f& b_scale) const {
	const float x = (position.x + scale.x / 2.0f - b_position.x + b_scale.x / 2.0f);
	const float y = (position.y + scale.y / 2.0f - b_position.y + b_scale.y / 2.0f);
	const float z = (position.z + scale.z / 2.0f - b_position.z + b_scale.z / 2.0f);
	return std::sqrt(x * x + y * y + z * z);
}

float transform::distance_to(const vector3f& b_position) const {
	const float x = (position.x + scale.x / 2.0f - b_position.x);
	const float y = (position.y + scale.y / 2.0f - b_position.y);
	const float z = (position.z + scale.z / 2.0f - b_position.z);
	return std::sqrt(x * x + y * y + z * z);
}

float transform::angle_to(const transform& b) const {
	const float y = position.y + scale.y / 2.0f - b.position.y + b.scale.y / 2.0f;
	const float x = position.x + scale.x / 2.0f - b.position.x + b.scale.y / 2.0f;
	float result = rad_to_deg(std::atan2(x, y));
	if (result > 180.0f) {
		result = 540.0f - result;
	} else {
		result = 180.0f - result;
	}
	return result;
}

float transform::angle_to(const vector3f& b_position, const vector3f& b_scale) const {
	const float y = position.y + scale.y / 2.0f - b_position.y + b_scale.y / 2.0f;
	const float x = position.x + scale.x / 2.0f - b_position.x + b_scale.y / 2.0f;
	float result = rad_to_deg(std::atan2(y, x));
	if (result > 180.0f) {
		result = 540.0f - result;
	} else {
		result = 180.0f - result;
	}
	return result;
}

float transform::angle_to(const vector3f& b_position) const {
	const float y = position.y + scale.y / 2.0f - b_position.y;
	const float x = position.x + scale.x / 2.0f - b_position.x;
	float result = rad_to_deg(std::atan2(y, x));
	if (result > 180.0f) {
		result = 540.0f - result;
	} else {
		result = 180.0f - result;
	}
	return result;
}

glm::mat4 transform_to_glm_mat4(const transform& t) {
	glm::mat4 matrix(1.0f);

	// todo: two transform classes?

	/*const vector3f origin = {
		t.position.x + t.scale.x / 2.0f,
		t.position.y + t.scale.y / 2.0f,
		t.position.z + t.scale.z / 2.0f
	};

	matrix = glm::translate(matrix, glm::vec3(origin.x, origin.y, origin.z));

	matrix = glm::rotate(matrix, deg_to_rad(t.rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
	matrix = glm::rotate(matrix, deg_to_rad(t.rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
	matrix = glm::rotate(matrix, deg_to_rad(-t.rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

	matrix = glm::translate(matrix, glm::vec3(-origin.x, -origin.y, -origin.z));
	matrix = glm::translate(matrix, glm::vec3(t.position.x, t.position.y, t.position.z));

	matrix = glm::scale(matrix, glm::vec3(t.scale.x, t.scale.y, t.scale.z));*/

	matrix = glm::translate(matrix, glm::vec3(t.position.x, t.position.y, t.position.z));

	matrix = glm::rotate(matrix, deg_to_rad(t.rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
	matrix = glm::rotate(matrix, deg_to_rad(t.rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
	matrix = glm::rotate(matrix, deg_to_rad(t.rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

	matrix = glm::scale(matrix, glm::vec3(t.scale.x, t.scale.y, t.scale.z));

	return matrix;
}

}
