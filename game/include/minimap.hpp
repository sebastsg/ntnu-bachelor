#pragma once

#include "surface.hpp"
#include "transform.hpp"

class world_state;

class world_minimap {
public:

	no::transform2 transform;

	world_minimap(const world_state& world);

	void update(no::vector2i tile);
	void draw() const;

private:

	const world_state& world;

	int texture = -1;
	no::surface surface;

};
