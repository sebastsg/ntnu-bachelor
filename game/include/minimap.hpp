#pragma once

#include "../config.hpp"

#if ENABLE_RENDERING

#include "surface.hpp"
#include "transform.hpp"
#include "timer.hpp"

struct world_tile;
class world_state;

class world_minimap {
public:

	no::transform2 transform;

	world_minimap(const world_state& world);

	void update(no::vector2i tile);
	void draw() const;

private:

	void refresh_background(no::vector2i tile, no::vector2i begin, no::vector2i end);
	void refresh_foreground(no::vector2i tile, no::vector2i begin, no::vector2i end);

	no::vector3f terrain_tile_color(const world_tile& tile) const;
	uint32_t color_to_pixel(const no::vector3f& color) const;

	const world_state& world;

	int texture = -1;
	no::surface surface;
	no::vector2i last_tile;
	no::timer timer;

};

#endif
