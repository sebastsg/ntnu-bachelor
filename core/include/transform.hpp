#pragma once

#include "math.hpp"

#include "GLM/gtc/matrix_transform.hpp"

namespace no {

enum class align_type {
	none,

	top_left,
	top_middle,
	top_right,

	left,
	middle,
	right,

	bottom_left,
	bottom_middle,
	bottom_right,
};

struct transform {

	vector3f position;
	vector3f rotation;
	vector3f scale = 1.0f;

	float center_x(float width) const;
	float center_y(float height) const;
	float center_z(float depth) const;
	vector3f center(const vector3f& size) const;

	void align(align_type alignment, const transform& parent, const vector3f& padding = 0.0f);

	bool collides_with(const transform& transform) const;
	bool collides_with(const vector3f& position, const vector3f& scale) const;
	bool collides_with(const vector3f& position) const;

	float distance_to(const transform& transform) const;
	float distance_to(const vector3f& position, const vector3f& scale) const;
	float distance_to(const vector3f& position) const;

	float angle_to(const transform& transform) const;
	float angle_to(const vector3f& position, const vector3f& scale) const;
	float angle_to(const vector3f& position) const;

};

class ray {
public:

	vector3f origin;
	vector3f direction;

};

glm::mat4 transform_to_glm_mat4(const transform& transform);

}
