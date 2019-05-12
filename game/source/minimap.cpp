#include "minimap.hpp"

#if ENABLE_RENDERING

#include "world.hpp"
#include "game_assets.hpp"

world_minimap::world_minimap(const world_state& world) : world{ world }, surface{ 64, 64, no::pixel_format::rgba, 0 } {
	texture = no::create_texture();
}

void world_minimap::update(no::vector2i tile) {
	no::vector2i begin = tile - 32;
	no::vector2i end = tile + 32;
	begin.x = std::max(0, begin.x);
	begin.y = std::max(0, begin.y);
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
			auto& tile = world.terrain.tile_at({ x, y });
			int grass = tile.corners_of_type(world_tile::grass);
			int dirt = tile.corners_of_type(world_tile::dirt);
			int water = tile.is_water();
			uint32_t color = 0xFFFFFFFF;
			if (grass > dirt) {
				color = 0xFF00FF00;
			}
			if (dirt > water) {
				color = 0xFF1199AA;
			}
			if (water > grass) {
				color = 0xFFEE2200;
			}
			surface.set(x - begin.x, y - begin.y, color);
		}
	}
	world.objects.for_each([&](const game_object* object) {
		no::vector2i object_tile = object->tile();
		if (std::abs(object_tile.x - tile.x) < 32 && std::abs(object_tile.y - tile.y) < 32) {
			switch (object->definition().type) {
			case game_object_type::character:
				surface.set(object_tile.x - begin.x, object_tile.y - begin.y, 0xFFEEEEEE);
				break;
			case game_object_type::item_spawn:
				surface.set(object_tile.x - begin.x, object_tile.y - begin.y, 0xFF2222FF);
				break;
			}
		}
	});
	no::load_texture(texture, surface);
}

void world_minimap::draw() const {
	no::bind_texture(texture);
	no::draw_shape(shapes().rectangle, transform);
}

#endif
