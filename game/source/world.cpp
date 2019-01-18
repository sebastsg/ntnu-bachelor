#include "world.hpp"
#include "assets.hpp"
#include "surface.hpp"

world_terrain::world_terrain(world_state& world) : world(world) {
	tile_heights.resize_and_reset(64, 0.0f);
	tile_heights.set(3, 3, 1.0f);
	tile_heights.set(3, 4, 2.0f);
	tile_heights.set(4, 3, 2.0f);
	tile_heights.set(4, 4, 2.0f);
	tile_heights.set(5, 4, 4.0f);
	tile_heights.set(5, 5, 3.0f);
	tile_heights.set(6, 4, 4.0f);
	tile_heights.set(6, 5, 5.0f);
}

bool world_terrain::is_out_of_bounds(no::vector2i tile) const {
	return tile_heights.is_out_of_bounds(tile.x, tile.y);
}

float world_terrain::elevation_at(no::vector2i tile) const {
	return tile_heights.at(tile.x, tile.y);
}

void world_terrain::set_elevation_at(no::vector2i tile, float elevation) {
	tile_heights.set(tile.x, tile.y, elevation);
}

void world_terrain::elevate_tile(no::vector2i tile, float amount) {
	if (is_out_of_bounds(tile)) {
		return;
	}
	tile_heights.add(tile.x, tile.y, amount);
	tile_heights.add(tile.x + 1, tile.y, amount);
	tile_heights.add(tile.x, tile.y + 1, amount);
	tile_heights.add(tile.x + 1, tile.y + 1, amount);
}

no::vector2i world_terrain::global_to_local_tile(no::vector2i tile) const {
	return tile - tile_heights.position();
}

no::vector2i world_terrain::local_to_global_tile(no::vector2i tile) const {
	return tile + tile_heights.position();
}

world_state::world_state() : terrain(*this) {
	camera.transform.scale.xy = 1.0f;
	camera.transform.rotation.x = 90.0f;
}

player_object& world_state::add_player(int id) {
	auto& player = players.emplace(id, player_object{ *this }).first->second;
	player.player_id = id;
	return player;
}

player_object& world_state::player(int id) {
	return players.find(id)->second;
}

player_object& world_state::my_player() {
	return players.find(my_player_id)->second;
}

const player_object& world_state::my_player() const {
	return players.find(my_player_id)->second;
}

no::vector2i world_state::world_position_to_tile_index(float x, float z) const {
	return { (int)std::floor(x), (int)std::floor(z) };
}

no::vector3f world_state::tile_index_to_world_position(int x, int z) const {
	return { (float)x, 0.0f, (float)z };
}

world_view::world_view(world_state& world) : world(world), player_render(world) {
	diffuse_shader = no::create_shader(no::asset_path("shaders/diffuse"));
	heightmap_shader = no::create_shader(no::asset_path("shaders/heightmap"));
	pick_shader = no::create_shader(no::asset_path("shaders/pick"));
	basic_mesh_shader = no::create_shader(no::asset_path("shaders/basic_mesh"));
	grass_texture = no::create_texture(no::surface(no::asset_path("textures/grass.png")), no::scale_option::nearest_neighbour, true);
	no::surface blank_surface = { 2, 2, no::pixel_format::rgba };
	blank_surface.clear(0xFFFFFFFF);
	blank_texture = no::create_texture(blank_surface);
	height_map.resize_and_reset({ world.terrain.tile_heights.columns(), world.terrain.tile_heights.rows() });
	height_map_pick.resize_and_reset({ world.terrain.tile_heights.columns(), world.terrain.tile_heights.rows() });
	refresh_terrain();
}

void world_view::draw() {
	draw_terrain();
	draw_players();
}

void world_view::draw_for_picking() {
	no::bind_shader(pick_shader);
	no::set_shader_view_projection(world.camera);
	no::transform transform;
	transform.position.x = (float)world.terrain.tile_heights.x();
	transform.position.z = (float)world.terrain.tile_heights.y();
	no::set_shader_model(transform);
	height_map_pick.bind();
	height_map_pick.draw();
}

void world_view::refresh_terrain() {
	height_map.set_y(world.terrain.tile_heights);
	height_map_pick.set_y(world.terrain.tile_heights);
	no::vector2i size = { height_map.width(), height_map.height() };
	for (int x = 0; x < size.x; x++) {
		for (int y = 0; y < size.y; y++) {
			auto& vertex = height_map.vertex(x, y);
			if (x % 2 == 0 && y % 2 == 0) {
				vertex.tex_coords = { 0.0f, 0.0f };
			} else if (x % 2 != 0 && y % 2 == 0) {
				vertex.tex_coords = { 1.0f, 0.0f };
			} else if (x % 2 == 0 && y % 2 != 0) {
				vertex.tex_coords = { 0.0f, 1.0f };
			} else if (x % 2 != 0 && y % 2 != 0) {
				vertex.tex_coords = { 1.0f, 1.0f };
			}
			height_map_pick.vertex(x, y).color.x = (float)x / 255.0f;
			height_map_pick.vertex(x, y).color.y = (float)y / 255.0f;
			height_map_pick.vertex(x, y).color.z = 1.0f;
		}
	}
	height_map.refresh();
	height_map_pick.refresh();
}

void world_view::draw_terrain() {
	no::bind_shader(heightmap_shader);
	no::set_shader_view_projection(world.camera);
	no::bind_texture(grass_texture);
	no::transform transform;
	transform.position.x = (float)world.terrain.tile_heights.x();
	transform.position.z = (float)world.terrain.tile_heights.y();
	no::set_shader_model(transform);
	height_map.bind();
	height_map.draw();
}

void world_view::draw_players() {
	no::bind_shader(diffuse_shader);
	no::set_shader_view_projection(world.camera);
	no::get_shader_variable("uni_Color").set({ 1.0f, 1.0f, 1.0f, 1.0f });
	no::get_shader_variable("uni_LightPosition").set(world.camera.transform.position + world.camera.offset());
	no::get_shader_variable("uni_LightColor").set(no::vector3f{ 1.0f, 1.0f, 1.0f });
	player_render.draw();
}

void world_view::draw_tile_highlight(no::vector2i tile) {
	if (world.terrain.is_out_of_bounds(tile) || world.terrain.is_out_of_bounds(tile + 1)) {
		return;
	}
	no::bind_shader(basic_mesh_shader);
	no::set_shader_view_projection(world.camera);

	no::static_mesh_vertex top_left;
	no::static_mesh_vertex top_right;
	no::static_mesh_vertex bottom_left;
	no::static_mesh_vertex bottom_right;

	float x = (float)tile.x;
	float z = (float)tile.y;

	top_left.position = { x, world.terrain.elevation_at(tile) + 0.01f, z };
	top_right.position = { x + 1.0f, world.terrain.elevation_at(tile + no::vector2i{ 1, 0 }) + 0.01f, z };
	bottom_left.position = { x, world.terrain.elevation_at(tile + no::vector2i{ 0, 1 }) + 0.01f, z + 1.0f };
	bottom_right.position = { x + 1.0f, world.terrain.elevation_at(tile + no::vector2i{ 1 }) + 0.01f, z + 1.0f };

	top_left.color = { 1.0f, 0.0f, 0.0f };
	top_right.color = { 1.0f, 0.0f, 0.0f };
	bottom_left.color = { 1.0f, 0.0f, 0.0f };
	bottom_right.color = { 1.0f, 0.0f, 0.0f };

	highlight_quad.set(top_left, top_right, bottom_left, bottom_right);

	no::bind_texture(blank_texture);
	highlight_quad.draw();
}
