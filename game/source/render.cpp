#include "render.hpp"
#include "assets.hpp"
#include "surface.hpp"

static no::vector2f uv_for_type(uint8_t type) {
	switch (type) {
	case 0: return { 0.0f, 0.0f };
	case 1: return { 0.5f, 0.0f };
	case 2: return { 0.0f, 0.5f };
	case 3: return { 0.5f, 0.5f };
	default: return 0.0f;
	}
}

player_renderer::player_renderer() {
	model.load<no::animated_mesh_vertex>(no::asset_path("models/player.nom"));
	sword_model.load<no::animated_mesh_vertex>(no::asset_path("models/sword.nom"));
	idle = model.index_of_animation("idle");
	run = model.index_of_animation("run");
}

void player_renderer::add(player_object* object) {
	int i = (int)players.size();
	auto& player = players.emplace_back(object, model);
	player.equip_event = player.object->events.equip.listen([i, this](const player_object::equip_event& event) {
		if (event.item_id != -1) {
			if (players[i].right_hand_attachment != -1) {
				players[i].model.detach(players[i].right_hand_attachment);
			}
			no::vector3f sword_offset = { 0.0761f, 0.5661f, 0.1151f };
			glm::quat sword_rotation = { 0.595f, -0.476f, -0.464f, -0.452f };
			players[i].right_hand_attachment = players[i].model.attach(15, sword_model, sword_offset, sword_rotation);
		} else {
			players[i].model.detach(players[i].right_hand_attachment);
		}
	});
}

void player_renderer::remove(player_object* object) {
	for (size_t i = 0; i < players.size(); i++) {
		if (players[i].object == object) {
			object->events.equip.ignore(players[i].equip_event);
			players.erase(players.begin() + i);
			break;
		}
	}
}

void player_renderer::draw() {
	for (auto& player : players) {
		if (player.object->is_moving()) {
			player.model.start_animation(run);
		} else {
			player.model.start_animation(idle);
		}
		player.model.animate();
		no::set_shader_model(player.object->transform);
		player.model.draw();
	}
}

decoration_renderer::decoration_renderer() {
	texture = no::create_texture(no::surface(no::asset_path("textures/decorations.png")), no::scale_option::nearest_neighbour, true);
}

decoration_renderer::object_data::object_data(decoration_object* object) : object(object) {
	model.load<no::static_textured_vertex>(no::asset_path(object->model));
}

void decoration_renderer::add(decoration_object* object) {
	decorations.emplace_back(object);
}

void decoration_renderer::remove(decoration_object* object) {
	for (size_t i = 0; i < decorations.size(); i++) {
		if (decorations[i].object == object) {
			decorations.erase(decorations.begin() + i);
			break;
		}
	}
}

void decoration_renderer::draw() {
	no::bind_texture(texture);
	for (auto& decoration : decorations) {
		no::set_shader_model(decoration.object->transform);
		decoration.model.bind();
		decoration.model.draw();
	}
}

world_view::world_view(world_state& world) : world(world) {
	camera.transform.scale.xy = 1.0f;
	camera.transform.rotation.x = 90.0f;
	diffuse_shader = no::create_shader(no::asset_path("shaders/diffuse"));
	pick_shader = no::create_shader(no::asset_path("shaders/pick"));
	static_textured_shader = no::create_shader(no::asset_path("shaders/static_textured"));
	tiles_texture = no::create_texture(no::surface(no::asset_path("textures/tiles.png")), no::scale_option::nearest_neighbour, true);
	no::surface surface = { 2, 2, no::pixel_format::rgba };
	surface.clear(0xFF0000FF);
	highlight_texture = no::create_texture(surface);

	height_map.build(4, world.terrain.size(), [this](int x, int y, std::vector<no::static_textured_vertex>& vertices, std::vector<unsigned short>& indices) {
		int i = (int)vertices.size();
		indices.push_back(i);
		indices.push_back(i + 1);
		indices.push_back(i + 2);
		indices.push_back(i);
		indices.push_back(i + 3);
		indices.push_back(i + 2);
		vertices.push_back({ { (float)x, 0.0f, (float)y }, 0.0f });
		vertices.push_back({ { (float)(x + 1), 0.0f, (float)y }, 0.0f });
		vertices.push_back({ { (float)(x + 1), 0.0f, (float)(y + 1) }, 0.0f });
		vertices.push_back({ { (float)x, 0.0f, (float)(y + 1) }, 0.0f });
	});

	height_map_pick.build(4, world.terrain.size(), [this](int x, int y, std::vector<no::pick_vertex>& vertices, std::vector<unsigned short>& indices) {
		int i = (int)vertices.size();
		indices.push_back(i);
		indices.push_back(i + 1);
		indices.push_back(i + 2);
		indices.push_back(i);
		indices.push_back(i + 3);
		indices.push_back(i + 2);
		vertices.push_back({ { (float)x, 0.0f, (float)y }, 0.0f });
		vertices.push_back({ { (float)(x + 1), 0.0f, (float)y }, 0.0f });
		vertices.push_back({ { (float)(x + 1), 0.0f, (float)(y + 1) }, 0.0f });
		vertices.push_back({ { (float)x, 0.0f, (float)(y + 1) }, 0.0f });
	});

	refresh_terrain();
}

void world_view::draw() {
	draw_terrain();
	draw_players();
	draw_decorations();
}

void world_view::draw_for_picking() {
	no::bind_shader(pick_shader);
	no::set_shader_view_projection(camera);
	no::transform transform;
	transform.position.x = (float)world.terrain.offset().x;
	transform.position.z = (float)world.terrain.offset().y;
	no::set_shader_model(transform);
	height_map_pick.bind();
	height_map_pick.draw();
}

void world_view::refresh_terrain() {
	height_map.for_each([this](int i, int x, int y, std::vector<no::static_textured_vertex>& vertices) {
		auto& tiles = world.terrain.tiles();
		auto& tile = tiles.at(tiles.x() + x, tiles.y() + y);
		auto& uv = uv_for_type(tile.type);
		vertices[i].position.y = tile.height;
		vertices[i].tex_coords = uv;
		if (i + 3 >= (int)vertices.size() || x + 1 >= tiles.columns() || y + 1 >= tiles.rows()) {
			return;
		}
		vertices[i + 1].position.y = tiles.at(tiles.x() + x + 1, tiles.y() + y).height;
		vertices[i + 2].position.y = tiles.at(tiles.x() + x + 1, tiles.y() + y + 1).height;
		vertices[i + 3].position.y = tiles.at(tiles.x() + x, tiles.y() + y + 1).height;
		vertices[i + 1].tex_coords = uv + no::vector2f{ 0.5f, 0.0f };
		vertices[i + 2].tex_coords = uv + no::vector2f{ 0.5f, 0.5f };
		vertices[i + 3].tex_coords = uv + no::vector2f{ 0.0f, 0.5f };
	});
	height_map_pick.for_each([this](int i, int x, int y, std::vector<no::pick_vertex>& vertices) {
		auto& tiles = world.terrain.tiles();
		auto& tile = tiles.at(tiles.x() + x, tiles.y() + y);
		vertices[i].position.y = tile.height;
		vertices[i].color.xy = { (float)x / 255.0f, (float)y / 255.0f };
		if (i + 3 >= (int)vertices.size() || x + 1 >= tiles.columns() || y + 1 >= tiles.rows()) {
			return;
		}
		vertices[i + 1].position.y = tiles.at(tiles.x() + x + 1, tiles.y() + y).height;
		vertices[i + 2].position.y = tiles.at(tiles.x() + x + 1, tiles.y() + y + 1).height;
		vertices[i + 3].position.y = tiles.at(tiles.x() + x, tiles.y() + y + 1).height;
		vertices[i + 1].color.xy = { (float)(x + 1) / 255.0f, (float)y / 255.0f };
		vertices[i + 2].color.xy = { (float)(x + 1) / 255.0f, (float)(y + 1) / 255.0f };
		vertices[i + 3].color.xy = { (float)x / 255.0f, (float)(y + 1) / 255.0f };
	});
	height_map.refresh();
	height_map_pick.refresh();
	world.terrain.set_clean();
}

void world_view::draw_terrain() {
	if (world.terrain.is_dirty()) {
		refresh_terrain();
	}
	no::bind_shader(static_textured_shader);
	no::set_shader_view_projection(camera);
	no::bind_texture(tiles_texture);
	no::transform transform;
	transform.position.x = (float)world.terrain.offset().x;
	transform.position.z = (float)world.terrain.offset().y;
	no::set_shader_model(transform);
	height_map.bind();
	height_map.draw();
}

void world_view::draw_players() {
	no::bind_shader(diffuse_shader);
	no::set_shader_view_projection(camera);
	no::get_shader_variable("uni_Color").set({ 1.0f, 1.0f, 1.0f, 1.0f });
	no::get_shader_variable("uni_LightPosition").set(camera.transform.position + camera.offset());
	no::get_shader_variable("uni_LightColor").set(no::vector3f{ 1.0f, 1.0f, 1.0f });
	players.draw();
}

void world_view::draw_decorations() {
	no::bind_shader(static_textured_shader);
	no::set_shader_view_projection(camera);
	decorations.draw();
}

void world_view::draw_tile_highlight(no::vector2i tile) {
	if (world.terrain.is_out_of_bounds(tile) || world.terrain.is_out_of_bounds(tile + 1)) {
		return;
	}
	no::bind_shader(static_textured_shader);
	no::set_shader_view_projection(camera);
	no::set_shader_model({});

	no::static_textured_vertex top_left;
	no::static_textured_vertex top_right;
	no::static_textured_vertex bottom_left;
	no::static_textured_vertex bottom_right;

	float x = (float)tile.x;
	float z = (float)tile.y;

	top_left.position = { x, world.terrain.elevation_at(tile) + 0.01f, z };
	top_right.position = { x + 1.0f, world.terrain.elevation_at(tile + no::vector2i{ 1, 0 }) + 0.01f, z };
	bottom_left.position = { x, world.terrain.elevation_at(tile + no::vector2i{ 0, 1 }) + 0.01f, z + 1.0f };
	bottom_right.position = { x + 1.0f, world.terrain.elevation_at(tile + no::vector2i{ 1 }) + 0.01f, z + 1.0f };

	top_left.tex_coords = { 0.0f, 0.0f };
	top_right.tex_coords = { 1.0f, 0.0f };
	bottom_left.tex_coords = { 0.0f, 1.0f };
	bottom_right.tex_coords = { 1.0f, 1.0f };

	highlight_quad.set(top_left, top_right, bottom_left, bottom_right);

	no::bind_texture(highlight_texture);
	highlight_quad.bind();
	highlight_quad.draw();
}
