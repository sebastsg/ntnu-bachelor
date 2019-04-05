#include "render.hpp"
#include "assets.hpp"
#include "surface.hpp"

static std::vector<unsigned short> filter_red_blue_indices(const no::model_data<no::animated_mesh_vertex>& data, float r, float b) {
	std::vector<unsigned short> result;
	result.reserve(data.shape.indices.size());
	for (int i = 0; i < (int)data.shape.indices.size(); i += 3) {
		bool keep = true;
		for (int j = 0; j < 3; j++) {
			int index = data.shape.indices[i + j];
			auto& vertex = data.shape.vertices[index];
			if (vertex.color.x > r && vertex.color.z < b) {
				keep = false;
				break;
			}
		}
		if (keep) {
			result.push_back(data.shape.indices[i]);
			result.push_back(data.shape.indices[i + 1]);
			result.push_back(data.shape.indices[i + 2]);
		}
	}
	return result;
}

character_renderer::character_renderer() {
	std::vector<game_object_definition> definitions = object_definitions().of_type(game_object_type::character);
	for (auto& definition : definitions) {
		if (character_models.find(definition.model) == character_models.end()) {
			character_models[definition.model] = {};
			no::model_data<no::animated_mesh_vertex> model_data;
			no::import_model(no::asset_path("models/" + definition.model + ".nom"), model_data);
			auto& model = character_models[definition.model];
			model.model.load(model_data);
			model.texture = no::create_texture({ no::asset_path("textures/" + model_data.texture + ".png") });
		}
	}
	for (auto& definition : definitions) {
		if (animators.find(definition.model) == animators.end()) {
			animators.emplace(definition.model, character_models[definition.model].model);
		}
	}

	//model_legs_hidden.shape.indices = filter_red_blue_indices(model_data, 0.9f, 0.1f);
	//model_body_hidden.shape.indices = filter_red_blue_indices(model_data, -0.1f, 0.9f);
	//model_legs_and_body_hidden.shape.indices = filter_red_blue_indices(model_data, 0.1f, -0.1f);
	//model_legs.load<no::animated_mesh_vertex>(model_legs_hidden);
	//model_body.load<no::animated_mesh_vertex>(model_body_hidden);
	//model_legs_and_body.load<no::animated_mesh_vertex>(model_legs_and_body_hidden);

	std::vector<item_definition> items = item_definitions().of_type(item_type::equipment);
	for (auto& item : items) {
		auto& equipment = equipment_models[item.id];
		equipment.model.load<no::animated_mesh_vertex>(no::asset_path("models/" + item.model + ".nom"));
		equipment.texture = no::create_texture({ no::asset_path("textures/" + item.model + ".png") });
	}
	for (auto& item : items) {
		equipment_animators.emplace(item.id, equipment_models[item.id].model);
	}
}

character_renderer::~character_renderer() {
	for (auto& model : character_models) {
		no::delete_texture(model.second.texture);
	}
	for (auto& equipment : equipment_models) {
		no::delete_texture(equipment.second.texture);
	}
}

void character_renderer::add(world_objects& objects, int object_id) {
	int i = (int)characters.size();
	auto& character = characters.emplace_back();
	character.object_id = object_id;
	character.model = objects.object(object_id).definition().model;
	character.animation = "idle";
	auto& object = *objects.character(object_id);
	character.events.equip = object.events.equip.listen([i, this](const item_instance& item) {
		on_equip(characters[i], item);
		characters[i].new_animation = true;
	});
	character.events.unequip = object.events.unequip.listen([i, this](const equipment_slot& slot) {
		on_unequip(characters[i], slot);
		characters[i].new_animation = true;
	});
	character.events.attack = object.events.attack.listen([&objects, i, this] {
		characters[i].animation = "attack";
		characters[i].new_animation = true;
		characters[i].play_once = true;
	});
	character.events.defend = object.events.defend.listen([i, this] {
		characters[i].animation = "defend";
		characters[i].new_animation = true;
		characters[i].play_once = true;
	});
	character.events.run = object.events.run.listen([&objects, i, this](bool running) {
		characters[i].animation = (running ? "run" : "idle");
		characters[i].new_animation = true;
		characters[i].play_once = false;
	});
	for (auto& item : object.equipment.items) {
		if (item.definition_id != -1) {
			on_equip(characters[i], item);
		}
	}
	character.animation_id = animators.find(character.model)->second.add();
}

void character_renderer::remove(character_object& object) {
	for (size_t i = 0; i < characters.size(); i++) {
		if (characters[i].object_id == object.object_id) {
			object.events.equip.ignore(characters[i].events.equip);
			object.events.unequip.ignore(characters[i].events.unequip);
			object.events.attack.ignore(characters[i].events.attack);
			object.events.defend.ignore(characters[i].events.defend);
			object.events.run.ignore(characters[i].events.run);
			for (int j = 0; j < (int)characters[i].equipments.size(); j++) {
				const auto& item = item_definitions().get(characters[i].equipments[j].item_id);
				auto& animator = equipment_animators.find(item.id)->second;
				animator.erase(characters[i].equipments[j].animation_id);
			}
			animators.find(characters[i].model)->second.erase(characters[i].animation_id);
			characters[i] = {};
			break;
		}
	}
}

void character_renderer::update(const no::bone_attachment_mapping_list& mappings, const world_objects& objects) {
	for (auto& character : characters) {
		if (character.object_id < 0) {
			continue;
		}
		auto& object = objects.object(character.object_id);
		auto character_object = objects.character(character.object_id);
		auto& animator = animators.find(object.definition().model)->second;
		auto& model = character_models[object.definition().model].model;
		auto& animation = animator.get(character.animation_id);
		if (!character.new_animation && character.play_once) {
			if (animator.will_be_reset(character.animation_id)) {
				character.animation = "idle";
				character.new_animation = true;
			}
		}
		if (character.animation == "idle" && character_object->in_combat()) {
			character.animation = "attack_idle";
			character.new_animation = true;
		}
		if (character.new_animation) {
			auto equipment = character_object->equipment.get(equipment_slot::right_hand).definition().equipment;
			if (equipment == equipment_type::spear) {
				character.animation += "_spear";
			}
			character.new_animation = false;
		}
		if (character.animation == "defend_spear") {
			character.animation = "attack_idle_spear"; // todo: need defend_spear animation
		}
		animation.transform = object.transform;
		animator.play(character.animation_id, character.animation, -1);
		for (auto& equipment : character.equipments) {
			auto& equipment_animator = equipment_animators.find(equipment.item_id)->second;
			auto& equipment_animation = equipment_animator.get(equipment.animation_id);
			equipment_animation.transform = object.transform;
			equipment_slot slot = item_definitions().get(equipment.item_id).slot;
			if (slot == equipment_slot::left_hand || slot == equipment_slot::right_hand) {
				// todo: proper mapping for root -> attach animation
				equipment_animator.play(equipment.animation_id, "default", -1);
				std::string equipment_name = equipment_models[equipment.item_id].model.name();
				equipment_animation.is_attachment = true;
				int char_animation = model.index_of_animation(character.animation);
				mappings.update(model, char_animation, equipment_name, equipment_animation.attachment);
				equipment_animation.root_transform = animation.transforms[equipment_animation.attachment.parent];
			} else {
				equipment_animator.play(equipment.animation_id, character.animation, -1);
			}
		}
	}
}

void character_renderer::draw() {
	for (auto& animator : animators) {
		animator.second.shader.bones = shader.bones;
		animator.second.animate();
		no::bind_texture(character_models[animator.first].texture);
		animator.second.draw();
	}
	for (auto& equipment : equipment_animators) {
		equipment.second.shader.bones = shader.bones;
		equipment.second.animate();
		no::bind_texture(equipment_models[equipment.first].texture);
		equipment.second.draw();
	}
}

void character_renderer::on_equip(character_animation& character, const item_instance& item) {
	equipment_slot slot = item_definitions().get(item.definition_id).slot;
	on_unequip(character, slot);
	auto equipment_model = equipment_models.find(item.definition_id);
	if (equipment_model == equipment_models.end()) {
		return;
	}
	auto& equipment_animator = equipment_animators.find(item.definition_id);
	character.equipments.push_back({ equipment_animator->second.add(), item.definition_id });
}

void character_renderer::on_unequip(character_animation& character, equipment_slot slot) {
	for (int i = 0; i < (int)character.equipments.size(); i++) {
		const auto& item = item_definitions().get(character.equipments[i].item_id);
		if (item.slot == slot) {
			auto& animator = equipment_animators.find(item.id)->second;
			animator.erase(character.equipments[i].animation_id);
			character.equipments.erase(character.equipments.begin() + i);
			break;
		}
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
		group.texture = no::create_texture({ no::asset_path("textures/" + group.model.texture_name() + ".png") });
	}
}

decoration_renderer::~decoration_renderer() {
	for (auto& group : groups) {
		no::delete_texture(group.texture);
	}
}

void decoration_renderer::add(const game_object& object) {
	for (auto& group : groups) {
		if (group.definition_id == object.definition_id) {
			group.objects.push_back(object.instance_id);
			break;
		}
	}
}

void decoration_renderer::remove(const game_object& object) {
	for (auto& group : groups) {
		for (size_t i = 0; i < group.objects.size(); i++) {
			if (group.objects[i] == object.instance_id) {
				group.objects.erase(group.objects.begin() + i);
				return;
			}
		}
	}
}

void decoration_renderer::draw(const world_objects& objects) {
	for (auto& group : groups) {
		no::bind_texture(group.texture);
		group.model.bind();
		for (int object_id : group.objects) {
			no::set_shader_model(objects.object(object_id).transform);
			group.model.draw();
		}
	}
}

object_pick_renderer::object_pick_renderer() {
	box.load(no::create_box_model_data<no::pick_vertex>([](const no::vector3f& vertex) {
		return no::pick_vertex{ vertex, 1.0f };
	}));
}

object_pick_renderer::~object_pick_renderer() {

}

void object_pick_renderer::draw(no::vector2i offset, const world_objects& objects) {
	if (!box.is_drawable()) {
		return;
	}
	box.bind();
	for (int object_id : object_ids) {
		const auto& object = objects.object(object_id);
		if (!object.pickable) {
			continue;
		}
		no::transform3 bbox = object.definition().bounding_box;
		no::transform3 transform;
		transform.position = object.transform.position + object.transform.scale * bbox.position;
		transform.scale = object.transform.scale * bbox.scale;
		transform.rotation.x = 270.0f;
		transform.rotation.z = object.transform.rotation.y;
		no::vector2f tile = (object.tile() + 1 - offset).to<float>();
		var_pick_color.set(no::vector3f{ tile.x / 255.0f, tile.y / 255.0f, 0.0f });
		no::set_shader_model(transform.to_matrix4_origin());
		box.draw();
	}
}

void object_pick_renderer::add(const game_object& object) {
	object_ids.push_back(object.instance_id);
}

void object_pick_renderer::remove(const game_object& object) {
	for (int i = 0; i < (int)object_ids.size(); i++) {
		if (object_ids[i] == object.instance_id) {
			object_ids.erase(object_ids.begin() + i);
			break;
		}
	}
}

world_view::world_view(world_state& world) : world(world) {
	mappings.load(no::asset_path("models/attachments.noma"));
	camera.transform.scale.xy = 1.0f;
	camera.transform.rotation.x = 90.0f;
	animate_diffuse_shader = no::create_shader(no::asset_path("shaders/animatediffuse"));
	light.var_position_animate = no::get_shader_variable("uni_LightPosition");
	light.var_color_animate = no::get_shader_variable("uni_LightColor");
	characters.shader.bones = no::get_shader_variable("uni_Bones");
	pick_shader = no::create_shader(no::asset_path("shaders/pick"));
	var_pick_color = no::get_shader_variable("uni_Color");
	pick_objects.var_pick_color = var_pick_color;
	static_diffuse_shader = no::create_shader(no::asset_path("shaders/staticdiffuse"));
	light.var_position_static = no::get_shader_variable("uni_LightPosition");
	light.var_color_static = no::get_shader_variable("uni_LightColor");
	fog.var_start = no::get_shader_variable("uni_FogStart");
	fog.var_distance = no::get_shader_variable("uni_FogDistance");
	var_color = no::get_shader_variable("uni_Color");
	no::surface temp_surface(no::asset_path("textures/tiles.png"));
	repeat_tile_under_row(temp_surface.data(), temp_surface.width(), temp_surface.height(), 1, 3, 1);
	repeat_tile_under_row(temp_surface.data(), temp_surface.width(), temp_surface.height(), 1, 4, 2);
	repeat_tile_under_row(temp_surface.data(), temp_surface.width(), temp_surface.height(), 2, 5, 1);
	repeat_tile_under_row(temp_surface.data(), temp_surface.width(), temp_surface.height(), 2, 6, 2);
	
	no::surface tile_surface = add_tile_borders(temp_surface.data(), temp_surface.width(), temp_surface.height());
	tileset.texture = no::create_texture(tile_surface, no::scale_option::nearest_neighbour, true);
	no::surface surface = { 2, 2, no::pixel_format::rgba };
	surface.clear(0xFFFFFFFF);
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

	add_object_id = world.objects.events.add.listen([this](const game_object& object) {
		add(object);
	});
	remove_object_id = world.objects.events.remove.listen([this](const game_object& object) {
		remove(object);
	});
	world.objects.for_each([this](game_object* object) {
		add(*object);
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

void world_view::draw() {
	light.position = camera.transform.position + camera.offset();
	draw_terrain();
	decorations.draw(world.objects);
	no::bind_shader(animate_diffuse_shader);
	no::set_shader_view_projection(camera);
	light.var_position_animate.set(light.position);
	light.var_color_animate.set(light.color);
	characters.update(mappings, world.objects);
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
	var_color.set(no::vector4f{ 1.0f });
	no::bind_texture(tileset.texture);
	no::transform3 transform;
	transform.position.x = (float)world.terrain.offset().x;
	transform.position.z = (float)world.terrain.offset().y;
	no::set_shader_model(transform);
	height_map.bind();
	height_map.draw();
}

void world_view::draw_for_picking() {
	no::bind_shader(pick_shader);
	no::set_shader_view_projection(camera);
	var_pick_color.set(no::vector3f{ 1.0f });
	no::transform3 transform;
	transform.position.x = (float)world.terrain.offset().x;
	transform.position.z = (float)world.terrain.offset().y;
	no::draw_shape(height_map_pick, transform);
	pick_objects.draw(world.terrain.offset(), world.objects);
}

void world_view::draw_tile_highlights(const std::vector<no::vector2i>& tiles, const no::vector4f& color) {
	no::bind_shader(static_diffuse_shader);
	no::set_shader_view_projection(camera);
	no::set_shader_model(no::transform3{});
	var_color.set(color);
	no::bind_texture(highlight_texture);
	static_object_vertex top_left;
	static_object_vertex top_right;
	static_object_vertex bottom_left;
	static_object_vertex bottom_right;
	top_left.tex_coords = { 0.0f, 0.0f };
	top_right.tex_coords = { 1.0f, 0.0f };
	bottom_left.tex_coords = { 0.0f, 1.0f };
	bottom_right.tex_coords = { 1.0f, 1.0f };
	for (no::vector2i tile : tiles) {
		if (world.terrain.is_out_of_bounds(tile) || world.terrain.is_out_of_bounds(tile + 1)) {
			continue;
		}
		float x = (float)tile.x;
		float z = (float)tile.y;
		top_left.position = { x, world.terrain.elevation_at(tile) + 0.01f, z };
		top_right.position = { x + 1.0f, world.terrain.elevation_at(tile + no::vector2i{ 1, 0 }) + 0.01f, z };
		bottom_left.position = { x, world.terrain.elevation_at(tile + no::vector2i{ 0, 1 }) + 0.01f, z + 1.0f };
		bottom_right.position = { x + 1.0f, world.terrain.elevation_at(tile + no::vector2i{ 1 }) + 0.01f, z + 1.0f };
		highlight_quad.set(top_left, top_right, bottom_left, bottom_right);
		highlight_quad.bind();
		highlight_quad.draw();
	}
}

void world_view::refresh_terrain() {
	height_map.for_each([this](int i, int x, int y, std::vector<static_object_vertex>& vertices) {
		auto& tiles = world.terrain.tiles();
		auto& tile = tiles.at(tiles.x() + x, tiles.y() + y);
		auto packed = world.terrain.autotiler.packed_corners(tile.corner(0), tile.corner(1), tile.corner(2), tile.corner(3));
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

void world_view::update_object_visibility() {
	world.objects.for_each([this](const game_object* object) {
		if (world.terrain.is_out_of_bounds(object->tile())) {
			remove(*object);
		} else {
			add(*object);
		}
	});
}

void world_view::add(const game_object& object) {
	ASSERT(object.instance_id >= 0);
	if (object.instance_id < 0) {
		return;
	}
	while (object.instance_id >= (int)object_visibilities.size()) {
		object_visibilities.emplace_back();
	}
	if (object_visibilities[object.instance_id]) {
		return;
	}
	object_visibilities[object.instance_id] = true;
	pick_objects.add(object);
	switch (object.definition().type) {
	case game_object_type::decoration:
		decorations.add(object);
		break;
	case game_object_type::character:
		characters.add(world.objects, object.instance_id);
		break;
	case game_object_type::item_spawn:
		break;
	case game_object_type::interactive:
		break;
	default:
		break;
	}
}

void world_view::remove(const game_object& object) {
	ASSERT(object.instance_id >= 0);
	if (object.instance_id < 0) {
		return;
	}
	while (object.instance_id >= (int)object_visibilities.size()) {
		object_visibilities.emplace_back();
	}
	if (!object_visibilities[object.instance_id]) {
		return;
	}
	object_visibilities[object.instance_id] = false;
	pick_objects.remove(object);
	switch (object.definition().type) {
	case game_object_type::decoration:
		decorations.remove(object);
		break;
	case game_object_type::character:
		characters.remove(*world.objects.character(object.instance_id));
		break;
	case game_object_type::item_spawn:
		break;
	case game_object_type::interactive:
		break;
	default:
		break;
	}
}

no::vector2f world_view::uv_step() const {
	return (float)tileset.grid / tileset.size.to<float>();
}

no::vector2f world_view::uv_for_type(no::vector2i index) const {
	float full_size = (float)(tileset.grid + tileset.border * 2);
	no::vector2f outer = full_size / tileset.size.to<float>();
	no::vector2f border_step = (float)tileset.border / tileset.size.to<float>();
	return {
		(float)index.x * outer.x + border_step.x,
		(float)index.y * outer.y + border_step.y
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
