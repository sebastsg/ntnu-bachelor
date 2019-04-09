#include "minimap.hpp"
#include "world.hpp"

world_minimap::world_minimap(const world_state& world) : world{ world }, surface{ 64, 64, no::pixel_format::rgba, 0 } {
	texture = no::create_texture();
}

void world_minimap::update(no::vector2i tile) {
	auto& tiles = world.terrain.tiles();
	no::vector2i begin = tile - 32;
	no::vector2i end = tile + 32;
	begin.x = std::max(0, begin.x);
	begin.y = std::max(0, begin.y);
	for (int x = begin.x; x < end.x; x++) {
		for (int y = begin.y; y < end.y; y++) {
			if (tiles.is_out_of_bounds(x, y)) {
				continue;
			}
			if (x < begin.x + 10 && (y < begin.y + 10 || y > end.y - 10)) {
				continue;
			}
			if (x > end.x - 10 && (y < begin.y + 10 || y > end.y - 10)) {
				continue;
			}
			int grass = tiles.at(x, y).corners_of_type(world_tile::grass);
			int dirt = tiles.at(x, y).corners_of_type(world_tile::dirt);
			int water = tiles.at(x, y).corners_of_type(world_tile::water);
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
			surface.set(object_tile.x - begin.x, object_tile.y - begin.y, 0xFFFFFFFF);
		}
	});
	no::load_texture(texture, surface);
}

void world_minimap::draw() const {
	no::bind_texture(texture);
	no::draw_shape(rectangle, transform);
}
