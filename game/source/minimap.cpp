#include "minimap.hpp"

#if ENABLE_RENDERING

#include "world.hpp"
#include "game_assets.hpp"

world_minimap::world_minimap(const world_state& world) : world{ world }, surface{ 64, 64, no::pixel_format::rgba, 0 } {
	texture = no::create_texture();
	timer.start();
}

void world_minimap::update(no::vector2i tile) {
	if (tile != last_tile || timer.milliseconds() > 200) {
		no::vector2i begin = tile - 32;
		no::vector2i end = tile + 32;
		begin.x = std::max(0, begin.x);
		begin.y = std::max(0, begin.y);
		refresh_background(tile, begin, end);
		refresh_foreground(tile, begin, end);
		no::load_texture(texture, surface);
		timer.start();
	}
	last_tile = tile;
}

void world_minimap::draw() const {
	no::bind_texture(texture);
	no::draw_shape(shapes().rectangle, transform);
}

void world_minimap::refresh_background(no::vector2i tile, no::vector2i begin, no::vector2i end) {
	for (int x = begin.x; x < end.x; x++) {
		for (int y = begin.y; y < end.y; y++) {
			if (world.terrain.is_out_of_bounds({ x, y })) {
				continue;
			}
			if (x < begin.x + 10 && (y < begin.y + 10 || y > end.y - 10)) {
				continue;
			}
			if (x > end.x - 10 && (y < begin.y + 10 || y > end.y - 10)) {
				continue;
			}
			const auto& center = world.terrain.tile_at({ x, y });
			const auto& left = world.terrain.tile_at({ x - 1, y });
			const auto& right = world.terrain.tile_at({ x + 1, y });
			const auto& up = world.terrain.tile_at({ x, y - 1 });
			const auto & down = world.terrain.tile_at({ x, y + 1 });
			no::vector3f color = terrain_tile_color(center);
			if (!left.equal_corners(center)) {
				color += terrain_tile_color(left) * 0.1f;
			}
			if (!right.equal_corners(center)) {
				color += terrain_tile_color(right) * 0.1f;
			}
			if (!up.equal_corners(center)) {
				color += terrain_tile_color(up) * 0.1f;
			}
			if (!down.equal_corners(center)) {
				color += terrain_tile_color(down) * 0.1f;
			}
			if (left.height > center.height) {
				color *= 1.0f - (left.height - center.height) / 7.0f;
			}
			if (right.height > center.height) {
				color *= 1.0f - (right.height - center.height) / 7.0f;
			}
			if (up.height > center.height) {
				color *= 1.0f - (up.height - center.height) / 7.0f;
			}
			if (down.height > center.height) {
				color *= 1.0f - (down.height - center.height) / 7.0f;
			}
			surface.set(x - begin.x, y - begin.y, color_to_pixel(color));
		}
	}
}

void world_minimap::refresh_foreground(no::vector2i tile, no::vector2i begin, no::vector2i end) {
	world.objects.for_each([&](const game_object * object) {
		no::vector2i object_tile = object->tile();
		if (std::abs(object_tile.x - tile.x) < 32 && std::abs(object_tile.y - tile.y) < 32) {
			switch (object->definition().type) {
			case game_object_type::decoration:
				surface.set(object_tile.x - begin.x, object_tile.y - begin.y, 0xFFCCCCAA);
				break;
			case game_object_type::character:
				surface.set(object_tile.x - begin.x, object_tile.y - begin.y, 0xFFEEEEEE);
				break;
			case game_object_type::item_spawn:
				surface.set(object_tile.x - begin.x, object_tile.y - begin.y, 0xFF2222FF);
				break;
			}
		}
	});
}

no::vector3f world_minimap::terrain_tile_color(const world_tile& tile) const {
	int grass = tile.corners_of_type(world_tile::grass);
	int dirt = tile.corners_of_type(world_tile::dirt);
	int stone = tile.corners_of_type(world_tile::stone);
	int snow = tile.corners_of_type(world_tile::snow);
	no::vector3f color{ 1.0f };
	if (grass > 0) {
		color = { 0.0f, 0.9f, 0.0f };
	}
	if (dirt > grass) {
		color = { 0.8f, 0.6f, 0.1f };
	}
	if (stone > dirt) {
		color = { 0.5f, 0.5f, 0.5f };
	}
	if (snow > stone) {
		color = { 0.9f, 0.9f, 0.9f };
	}
	if (tile.water_height > tile.height && tile.is_water()) {
		color = { 0.0f, 0.1f, 0.9f };
	}
	return color;
}

uint32_t world_minimap::color_to_pixel(const no::vector3f& color) const {
	uint8_t r = (uint8_t)(std::min(color.x, 1.0f) * 255.0f);
	uint8_t g = (uint8_t)(std::min(color.y, 1.0f) * 255.0f);
	uint8_t b = (uint8_t)(std::min(color.z, 1.0f) * 255.0f);
	return (uint32_t)((0xFF << 24) + (b << 16) + (g << 8) + r);
}

#endif
