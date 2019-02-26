#include "transform.hpp"

namespace no {

glm::mat4 transform2::to_matrix4() const {
	const vector2f origin = position + scale / 2.0f;
	glm::mat4 matrix(1.0f);
	matrix = glm::translate(matrix, { origin.x, origin.y, 0.0f });
	matrix = glm::rotate(matrix, deg_to_rad(-rotation), { 0.0f, 0.0f, 1.0f });
	matrix = glm::translate(matrix, { -origin.x, -origin.y, 0.0f });

	matrix = glm::translate(matrix, { position.x, position.y, 0.0f });

	matrix = glm::scale(matrix, { scale.x, scale.y, 1.0f });
	return matrix;
}

float transform2::center_x(float width) const {
	return position.x + scale.x / 2.0f - width / 2.0f;
}

float transform2::center_y(float height) const {
	return position.y + scale.y / 2.0f - height / 2.0f;
}

vector2f transform2::center(const vector2f& size) const {
	return position + scale / 2.0f - size / 2.0f;
}

bool transform2::collides_with(const transform2& b) const {
	if (position.x > b.position.x + b.scale.x || position.x + scale.x < b.position.x) {
		return false;
	}
	if (position.y > b.position.y + b.scale.y || position.y + scale.y < b.position.y) {
		return false;
	}
	return true;
}

bool transform2::collides_with(const vector2f& b_position, const vector2f& b_scale) const {
	if (position.x > b_position.x + b_scale.x || position.x + scale.x < b_position.x) {
		return false;
	}
	if (position.y > b_position.y + b_scale.y || position.y + scale.y < b_position.y) {
		return false;
	}
	return true;
}

bool transform2::collides_with(const vector2f& b_position) const {
	if (b_position.x < position.x || b_position.x > position.x + scale.x) {
		return false;
	}
	if (b_position.y < position.y || b_position.y > position.y + scale.y) {
		return false;
	}
	return true;
}

float transform2::distance_to(const transform2& b) const {
	const float x = (position.x + scale.x / 2.0f - b.position.x + b.scale.x / 2.0f);
	const float y = (position.y + scale.y / 2.0f - b.position.y + b.scale.y / 2.0f);
	return std::sqrt(x * x + y * y);
}

float transform2::distance_to(const vector2f& b_position, const vector2f& b_scale) const {
	const float x = (position.x + scale.x / 2.0f - b_position.x + b_scale.x / 2.0f);
	const float y = (position.y + scale.y / 2.0f - b_position.y + b_scale.y / 2.0f);
	return std::sqrt(x * x + y * y);
}

float transform2::distance_to(const vector2f& b_position) const {
	const float x = (position.x + scale.x / 2.0f - b_position.x);
	const float y = (position.y + scale.y / 2.0f - b_position.y);
	return std::sqrt(x * x + y * y);
}

float transform2::angle_to(const transform2& b) const {
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

float transform2::angle_to(const vector2f& b_position, const vector2f& b_scale) const {
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

float transform2::angle_to(const vector2f& b_position) const {
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

void transform2::align(align_type alignment, const transform2& parent, const vector2f& padding) {
	switch (alignment) {
	case align_type::none:
		break;
	case align_type::top_left:
		position = parent.position;
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

glm::mat4 transform3::to_matrix4() const {
	glm::mat4 matrix(1.0f);
	matrix = glm::translate(matrix, { position.x, position.y, position.z });

	matrix = glm::rotate(matrix, deg_to_rad(rotation.x), { 1.0f, 0.0f, 0.0f });
	matrix = glm::rotate(matrix, deg_to_rad(rotation.y), { 0.0f, 1.0f, 0.0f });
	matrix = glm::rotate(matrix, deg_to_rad(rotation.z), { 0.0f, 0.0f, 1.0f });

	matrix = glm::scale(matrix, { scale.x, scale.y, scale.z });
	return matrix;
}

float transform3::center_x(float width) const {
	return position.x + scale.x / 2.0f - width / 2.0f;
}

float transform3::center_y(float height) const {
	return position.y + scale.y / 2.0f - height / 2.0f;
}

float transform3::center_z(float depth) const {
	return position.z + scale.z / 2.0f - depth / 2.0f;
}

vector3f transform3::center(const vector3f& size) const {
	return {
		position.x + scale.x / 2.0f - size.x / 2.0f,
		position.y + scale.y / 2.0f - size.y / 2.0f,
		position.z + scale.z / 2.0f - size.z / 2.0f
	};
}

bool transform3::collides_with(const transform3& b) const {
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

bool transform3::collides_with(const vector3f& b_position, const vector3f& b_scale) const {
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

bool transform3::collides_with(const vector3f& b_position) const {
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

float transform3::distance_to(const transform3& b) const {
	const float x = (position.x + scale.x / 2.0f - b.position.x + b.scale.x / 2.0f);
	const float y = (position.y + scale.y / 2.0f - b.position.y + b.scale.y / 2.0f);
	const float z = (position.z + scale.z / 2.0f - b.position.z + b.scale.z / 2.0f);
	return std::sqrt(x * x + y * y + z * z);
}

float transform3::distance_to(const vector3f& b_position, const vector3f& b_scale) const {
	const float x = (position.x + scale.x / 2.0f - b_position.x + b_scale.x / 2.0f);
	const float y = (position.y + scale.y / 2.0f - b_position.y + b_scale.y / 2.0f);
	const float z = (position.z + scale.z / 2.0f - b_position.z + b_scale.z / 2.0f);
	return std::sqrt(x * x + y * y + z * z);
}

float transform3::distance_to(const vector3f& b_position) const {
	const float x = (position.x + scale.x / 2.0f - b_position.x);
	const float y = (position.y + scale.y / 2.0f - b_position.y);
	const float z = (position.z + scale.z / 2.0f - b_position.z);
	return std::sqrt(x * x + y * y + z * z);
}

}
