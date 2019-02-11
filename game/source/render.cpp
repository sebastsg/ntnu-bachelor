#include "render.hpp"
#include "assets.hpp"
#include "surface.hpp"

character_renderer::character_renderer(world_view& world) : world(world) {
	player_texture = no::create_texture(no::surface(no::asset_path("textures/character.png")), no::scale_option::nearest_neighbour, true);
	model.load<no::animated_mesh_vertex>(no::asset_path("models/character.nom"));
	idle = model.index_of_animation("idle");
	run = model.index_of_animation("run");
	equipments[0] = new no::model();
	equipments[0]->load<no::animated_mesh_vertex>(no::asset_path("models/sword.nom"));
	equipments[1] = new no::model();
	equipments[1]->load<no::animated_mesh_vertex>(no::asset_path("models/shield.nom"));
}

character_renderer::~character_renderer() {
	for (auto& equipment : equipments) {
		delete equipment.second;
	}
	no::delete_texture(player_texture);
}

void character_renderer::add(character_object* object) {
	int i = (int)characters.size();
	auto& character = characters.emplace_back(object, model);
	character.equip_event = character.object->events.equip.listen([i, this](const character_object::equip_event& event) {
		auto& attachment = characters[i].attachments.find(event.slot);
		if (attachment != characters[i].attachments.end()) {
			characters[i].model.detach(attachment->second);
			characters[i].attachments.erase(attachment->first);
		}
		if (event.item_id != -1) {
			characters[i].attachments[event.slot] = characters[i].model.attach(*equipments[event.item_id], world.mappings);
		}
	});
}

void character_renderer::remove(character_object* object) {
	for (size_t i = 0; i < characters.size(); i++) {
		if (characters[i].object == object) {
			object->events.equip.ignore(characters[i].equip_event);
			characters.erase(characters.begin() + i);
			break;
		}
	}
}

void character_renderer::draw() {
	no::bind_texture(player_texture);
	for (auto& character : characters) {
		if (character.object->is_moving()) {
			character.model.start_animation(run);
		} else {
			character.model.start_animation(idle);
		}
		character.model.animate();
		no::set_shader_model(character.object->transform);
		character.model.draw();
	}
}

decoration_renderer::decoration_renderer() {
	for (int i = 0; i < object_definitions().count(); i++) {
		auto& definition = object_definitions().get(i);
		if (definition.type != game_object_type::decoration) {
			continue;
		}
		auto& group = groups.emplace_back();
		group.definition_id = definition.id;
		group.model.load<static_object_vertex>(no::asset_path("models/" + definition.model + ".nom"));
		group.texture = no::create_texture(no::surface(no::asset_path("textures/" + group.model.texture_name() + ".png")), no::scale_option::nearest_neighbour, true);
	}
}

decoration_renderer::~decoration_renderer() {
	for (auto& group : groups) {
		no::delete_texture(group.texture);
	}
}

void decoration_renderer::add(decoration_object* object) {
	for (auto& group : groups) {
		if (group.definition_id == object->definition().id) {
			group.objects.push_back(object);
			break;
		}
	}
}

void decoration_renderer::remove(decoration_object* object) {
	for (auto& group : groups) {
		for (size_t i = 0; i < group.objects.size(); i++) {
			if (group.objects[i] == object) {
				group.objects.erase(group.objects.begin() + i);
				return;
			}
		}
	}
}

void decoration_renderer::draw() {
	for (auto& group : groups) {
		no::bind_texture(group.texture);
		group.model.bind();
		for (auto& object : group.objects) {
			no::set_shader_model(object->transform);
			group.model.draw();
		}
	}
}

world_view::world_view(world_state& world) : world(world), characters(*this) {
	mappings.load(no::asset_path("models/attachments.noma"));
	camera.transform.scale.xy = 1.0f;
	camera.transform.rotation.x = 90.0f;
	animate_diffuse_shader = no::create_shader(no::asset_path("shaders/animatediffuse"));
	light.var_position_animate = no::get_shader_variable("uni_LightPosition");
	light.var_color_animate = no::get_shader_variable("uni_LightColor");
	pick_shader = no::create_shader(no::asset_path("shaders/pick"));
	static_diffuse_shader = no::create_shader(no::asset_path("shaders/staticdiffuse"));
	light.var_position_static = no::get_shader_variable("uni_LightPosition");
	light.var_color_static = no::get_shader_variable("uni_LightColor");
	fog.var_start = no::get_shader_variable("uni_FogStart");
	fog.var_distance = no::get_shader_variable("uni_FogDistance");
	no::surface temp_surface(no::asset_path("textures/tiles.png"));
	repeat_tile_under_row(temp_surface.data(), temp_surface.width(), temp_surface.height(), 1, 3, 1);
	repeat_tile_under_row(temp_surface.data(), temp_surface.width(), temp_surface.height(), 1, 4, 2);
	repeat_tile_under_row(temp_surface.data(), temp_surface.width(), temp_surface.height(), 2, 5, 1);
	repeat_tile_under_row(temp_surface.data(), temp_surface.width(), temp_surface.height(), 2, 6, 2);
	no::surface tile_surface = add_tile_borders(temp_surface.data(), temp_surface.width(), temp_surface.height());
	tileset.texture = no::create_texture(tile_surface, no::scale_option::nearest_neighbour, false);
	no::surface surface = { 2, 2, no::pixel_format::rgba };
	surface.clear(0xFF0000FF);
	highlight_texture = no::create_texture(surface);

	height_map.build(4, world.terrain.size(), [this](int x, int y, std::vector<static_object_vertex>& vertices, std::vector<unsigned short>& indices) {
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

	add_object_id = world.objects.events.add.listen([this](const world_objects::add_event& event) {
		add(event.object);
	});
	remove_object_id = world.objects.events.remove.listen([this](const world_objects::remove_event& event) {
		remove(event.object);
	});
	world.objects.for_each([this](game_object* object) {
		add(object);
	});
}

world_view::~world_view() {
	world.objects.events.add.ignore(add_object_id);
	world.objects.events.remove.ignore(remove_object_id);
	no::delete_shader(animate_diffuse_shader);
	no::delete_shader(static_diffuse_shader);
	no::delete_shader(pick_shader);
	no::delete_texture(highlight_texture);
	no::delete_texture(tileset.texture);
}

void world_view::add(game_object* object) {
	if (!object) {
		return;
	}
	switch (object->definition().type) {
	case game_object_type::decoration:
		decorations.add((decoration_object*)object);
		break;
	case game_object_type::character:
		characters.add((character_object*)object);
		break;
	case game_object_type::item_spawn:
		break;
	case game_object_type::interactive:
		break;
	default:
		break;
	}
}

void world_view::remove(game_object* object) {
	if (!object) {
		return;
	}
	switch (object->definition().type) {
	case game_object_type::decoration:
		decorations.remove((decoration_object*)object);
		break;
	case game_object_type::character:
		characters.remove((character_object*)object);
		break;
	case game_object_type::item_spawn:
		break;
	case game_object_type::interactive:
		break;
	default:
		break;
	}
}

void world_view::draw() {
	// camera.transform.position + camera.offset()
	light.position = camera.transform.position + camera.offset();
	draw_terrain();
	decorations.draw();
	no::bind_shader(animate_diffuse_shader);
	no::set_shader_view_projection(camera);
	light.var_position_animate.set(light.position);
	light.var_color_animate.set(light.color);
	characters.draw();
}

void world_view::draw_terrain() {
	if (world.terrain.is_dirty()) {
		refresh_terrain();
	}
	no::bind_shader(static_diffuse_shader);
	no::set_shader_view_projection(camera);
	light.var_position_static.set(light.position);
	light.var_color_static.set(light.color);
	fog.var_start.set(fog.start);
	fog.var_distance.set(fog.distance);
	no::bind_texture(tileset.texture);
	no::transform transform;
	transform.position.x = (float)world.terrain.offset().x;
	transform.position.z = (float)world.terrain.offset().y;
	no::set_shader_model(transform);
	height_map.bind();
	height_map.draw();
}

void world_view::draw_for_picking() {
	no::bind_shader(pick_shader);
	no::set_shader_view_projection(camera);
	no::transform transform;
	transform.position.x = (float)world.terrain.offset().x;
	transform.position.z = (float)world.terrain.offset().y;
	no::draw_shape(height_map_pick, transform);
}

void world_view::draw_tile_highlight(no::vector2i tile) {
	if (world.terrain.is_out_of_bounds(tile) || world.terrain.is_out_of_bounds(tile + 1)) {
		return;
	}
	no::bind_shader(static_diffuse_shader);
	no::set_shader_view_projection(camera);
	no::set_shader_model({});

	static_object_vertex top_left;
	static_object_vertex top_right;
	static_object_vertex bottom_left;
	static_object_vertex bottom_right;

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

void world_view::refresh_terrain() {
	height_map.for_each([this](int i, int x, int y, std::vector<static_object_vertex>& vertices) {
		auto& tiles = world.terrain.tiles();
		auto& tile = tiles.at(tiles.x() + x, tiles.y() + y);
		auto packed = world.terrain.autotiler.packed_corners(tile.corners[0], tile.corners[1], tile.corners[2], tile.corners[3]);
		no::vector2f uv = uv_for_type(world.terrain.autotiler.uv_index(packed));
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

		no::vector3f a_to_c = vertices[i + 2].position - vertices[i].position;
		no::vector3f b_to_d = vertices[i + 1].position - vertices[i + 3].position;
		no::vector3f normal = a_to_c.cross(b_to_d).normalized();
		vertices[i].normal = normal;
		vertices[i + 1].normal = normal;
		vertices[i + 2].normal = normal;
		vertices[i + 3].normal = normal;
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

no::vector2f world_view::uv_for_type(no::vector2i index) const {
	//float col = (float)(uv_index % tileset.per_row);
	//float row = (float)(uv_index / tileset.per_row);
	float col = (float)index.x;
	float row = (float)index.y;
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

void world_view::repeat_tile_under_row(uint32_t* pixels, int width, int height, int tile, int new_row, int row) {
	for (int col = 0; col < tileset.per_row; col++) {
		int col_top_left = new_row * tileset.grid * width + col * tileset.grid;
		for (int y = 0; y < tileset.grid; y++) {
			// main tile:
			memcpy(pixels + col_top_left + y * width, pixels + y * width + tile * tileset.grid, sizeof(uint32_t) * tileset.grid);
			// overlay foreground:
			for (int x = 0; x < tileset.grid; x++) {
				uint32_t source = pixels[(tileset.grid * row + y) * width + col * tileset.grid + x];
				if (source != 0x00FFFFFF) {
					pixels[col_top_left + y * width + x] = source;
				}
			}
		}
	}
}
