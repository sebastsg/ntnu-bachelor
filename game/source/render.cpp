#include "render.hpp"
#include "assets.hpp"
#include "surface.hpp"

player_renderer::player_renderer(world_view& world) : world(world) {
	player_texture = no::create_texture(no::surface(no::asset_path("textures/player.png")), no::scale_option::nearest_neighbour, true);
	model.load<no::animated_mesh_vertex>(no::asset_path("models/player.nom"));
	idle = model.index_of_animation("idle");
	run = model.index_of_animation("run");
	equipments[0] = new no::model();
	equipments[0]->load<no::animated_mesh_vertex>(no::asset_path("models/sword.nom"));
	equipments[1] = new no::model();
	equipments[1]->load<no::animated_mesh_vertex>(no::asset_path("models/shield.nom"));
}

player_renderer::~player_renderer() {
	for (auto& equipment : equipments) {
		delete equipment.second;
	}
	no::delete_texture(player_texture);
}

void player_renderer::add(player_object* object) {
	int i = (int)players.size();
	auto& player = players.emplace_back(object, model);
	player.equip_event = player.object->events.equip.listen([i, this](const player_object::equip_event& event) {
		auto& attachment = players[i].attachments.find(event.slot);
		if (attachment != players[i].attachments.end()) {
			players[i].model.detach(attachment->second);
			players[i].attachments.erase(attachment->first);
		}
		if (event.item_id != -1) {
			players[i].attachments[event.slot] = players[i].model.attach(*equipments[event.item_id], world.mappings);
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
	no::bind_texture(player_texture);
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

world_view::world_view(world_state& world) : world(world), players(*this) {
	mappings.load(no::asset_path("models/attachments.noma"));
	camera.transform.scale.xy = 1.0f;
	camera.transform.rotation.x = 90.0f;
	diffuse_shader = no::create_shader(no::asset_path("shaders/diffuse"));
	pick_shader = no::create_shader(no::asset_path("shaders/pick"));
	static_textured_shader = no::create_shader(no::asset_path("shaders/static_textured"));
	no::surface temp_surface(no::asset_path("textures/tiles.png"));
	no::surface tile_surface = add_tile_borders(temp_surface.data(), temp_surface.width(), temp_surface.height());
	tileset.texture = no::create_texture(tile_surface, no::scale_option::nearest_neighbour, false);
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
		no::vector2f uv = uv_for_type(tile.type);
		no::vector2f step = uv_step();
		vertices[i].position.y = tile.height;
		vertices[i].tex_coords = uv;
		if (i + 3 >= (int)vertices.size() || x + 1 >= tiles.columns() || y + 1 >= tiles.rows()) {
			return;
		}
		vertices[i + 1].position.y = tiles.at(tiles.x() + x + 1, tiles.y() + y).height;
		vertices[i + 2].position.y = tiles.at(tiles.x() + x + 1, tiles.y() + y + 1).height;
		vertices[i + 3].position.y = tiles.at(tiles.x() + x, tiles.y() + y + 1).height;
		vertices[i + 1].tex_coords = uv + no::vector2f{ step.x, 0.0f };
		vertices[i + 2].tex_coords = uv + step;
		vertices[i + 3].tex_coords = uv + no::vector2f{ 0.0f, step.y };
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

no::vector2f world_view::uv_step() const {
	return (float)tileset.grid / tileset.size.to<float>();
}

no::vector2f world_view::uv_for_type(uint8_t type) const {
	float col = (float)(type % tileset.per_row);
	float row = (float)(type / tileset.per_row);
	float full_size = (float)(tileset.grid + tileset.border * 2);
	no::vector2f outer = full_size / tileset.size.to<float>();
	no::vector2f border_step = (float)tileset.border / tileset.size.to<float>();
	return {
		col * outer.x + border_step.x,
		row * outer.y + border_step.y
	};
}

// this exists to avoid issues with both multisampling and mipmapping
// todo: wouldn't hurt to make this prettier.
no::surface world_view::add_tile_borders(uint32_t* pixels, int width, int height) {
	int grid = tileset.grid;
	int border_size = tileset.border;
	int new_width = width + (width / grid) * border_size * 2;
	int new_height = height + (height / grid) * border_size * 2;
	uint32_t* result = new uint32_t[new_width * new_height];
	memset(result, 0x00, new_width * new_height * sizeof(uint32_t));
	tileset.size = { new_width, new_height };
	for (int tile_y = 0; tile_y < tileset.per_column; tile_y++) {
		for (int tile_x = 0; tile_x < tileset.per_row; tile_x++) {
			int i_top_left = tile_y * grid * width + tile_x * grid;
			int j_column = tile_x * (grid + border_size * 2);
			int j_row = tile_y * (grid + border_size * 2);
			int j_inner = (j_row + border_size) * new_width + j_column + border_size;
			for (int tile_row = 0; tile_row < grid; tile_row++) {
				memcpy(result + j_inner + tile_row * new_width, pixels + i_top_left + tile_row * width, sizeof(uint32_t) * grid);
			}
			int j_top_left = j_row * new_width + j_column;
			for (int border = 0; border < border_size; border++) {
				// top
				for (int edge_border = 0; edge_border < border_size; edge_border++) {
					result[j_top_left + border + edge_border * new_width] = pixels[i_top_left];
					result[j_top_left + border_size + border + grid + edge_border * new_width] = pixels[i_top_left + grid - 1];
				}
				memcpy(result + j_top_left + border * new_width + border_size, pixels + i_top_left, sizeof(uint32_t) * grid);
				// bottom
				for (int edge_border = 0; edge_border < border_size; edge_border++) {
					result[j_top_left + (grid + border_size) * new_width + border + edge_border * new_width] = pixels[i_top_left + (grid - 1) * width];
					result[j_top_left + (grid + border_size) * new_width + border_size + border + grid + edge_border * new_width] = pixels[i_top_left + (grid - 1) * width + grid - 1];
				}
				memcpy(result + j_top_left + (grid + border_size + border) * new_width + border_size, pixels + i_top_left + (grid - 1) * width, sizeof(uint32_t) * grid);
				// left
				for (int border_y = 0; border_y < grid; border_y++) {
					result[j_top_left + (border_y + border_size) * new_width + border] = pixels[i_top_left + border_y * width];
				}
				// right
				for (int border_y = 0; border_y < grid; border_y++) {
					result[j_top_left + (border_y + border_size) * new_width + border + grid + border_size] = pixels[i_top_left + border_y * width + grid - 1];
				}
			}
		}
	}
	return no::surface(result, new_width, new_height, no::pixel_format::rgba, no::surface::construct_by::move);
}

void world_view::draw_terrain() {
	if (world.terrain.is_dirty()) {
		refresh_terrain();
	}
	no::bind_shader(static_textured_shader);
	no::set_shader_view_projection(camera);
	no::bind_texture(tileset.texture);
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
