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

struct transform2 {

	vector2f position;
	float rotation = 0.0f;
	vector2f scale = 1.0f;

	glm::mat4 to_matrix4() const;

	float center_x(float width) const;
	float center_y(float height) const;
	vector2f center(const vector2f& size) const;

	bool collides_with(const transform2& transform) const;
	bool collides_with(const vector2f& position, const vector2f& scale) const;
	bool collides_with(const vector2f& position) const;

	float distance_to(const transform2& transform) const;
	float distance_to(const vector2f& position, const vector2f& scale) const;
	float distance_to(const vector2f& position) const;

	float angle_to(const transform2& transform) const;
	float angle_to(const vector2f& position, const vector2f& scale) const;
	float angle_to(const vector2f& position) const;

	void align(align_type alignment, const transform2& parent, const vector2f& padding);

};

struct transform3 {

	vector3f position;
	vector3f rotation;
	vector3f scale = 1.0f;

	glm::mat4 to_matrix4() const;
	
	float center_x(float width) const;
	float center_y(float height) const;
	float center_z(float depth) const;
	vector3f center(const vector3f& size) const;

	bool collides_with(const transform3& transform) const;
	bool collides_with(const vector3f& position, const vector3f& scale) const;
	bool collides_with(const vector3f& position) const;

	float distance_to(const transform3& transform) const;
	float distance_to(const vector3f& position, const vector3f& scale) const;
	float distance_to(const vector3f& position) const;

};

class ray {
public:

	vector3f origin;
	vector3f direction;

};

}
