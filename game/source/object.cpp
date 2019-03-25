#include "object.hpp"
#include "world.hpp"
#include "character.hpp"
#include "assets.hpp"
#include "loop.hpp"
#include "assets.hpp"

namespace global {
static game_object_definition_list object_definitions;
}

game_object_definition_list& object_definitions() {
	return global::object_definitions;
}

const game_object_definition& object_definition(int id) {
	return global::object_definitions.get(id);
}

game_object_definition_list::game_object_definition_list() {
	invalid.name = "Invalid object";
	no::post_configure_event().listen([this] {
		load(no::asset_path("objects.data"));
	});
}

void game_object_definition_list::save(const std::string& path) const {
	no::io_stream stream;
	stream.write((int32_t)definitions.size());
	for (auto& definition : definitions) {
		stream.write((int32_t)definition.type);
		stream.write((int32_t)definition.id);
		stream.write(definition.name);
		stream.write(definition.model);
		stream.write(definition.bounding_box);
		stream.write<int32_t>(definition.dialogue_id);
		stream.write(definition.description);
	}
	no::file::write(path, stream);
}

void game_object_definition_list::load(const std::string& path) {
	definitions.clear();
	no::io_stream stream;
	no::file::read(path, stream);
	if (stream.write_index() == 0) {
		return;
	}
	int32_t count = stream.read<int32_t>();
	for (int32_t i = 0; i < count; i++) {
		game_object_definition definition;
		definition.type = (game_object_type)stream.read<int32_t>();
		definition.id = stream.read<int32_t>();
		definition.name = stream.read<std::string>();
		definition.model = stream.read<std::string>();
		definition.bounding_box = stream.read<no::transform3>();
		definition.dialogue_id = stream.read<int32_t>();
		definition.description = stream.read<std::string>();
		definitions.push_back(definition);
	}
}

game_object_definition& game_object_definition_list::get(int id) {
	if (id >= (int)definitions.size() || id < 0) {
		return invalid;
	}
	return definitions[id];
}

const game_object_definition& game_object_definition_list::get(int id) const {
	if (id >= (int)definitions.size() || id < 0) {
		return invalid;
	}
	return definitions[id];
}

int game_object_definition_list::count() const {
	return (int)definitions.size();
}

bool game_object_definition_list::add(const game_object_definition& definition) {
	if (!conflicts(definition)) {
		definitions.push_back(definition);
		return true;
	}
	return false;
}

bool game_object_definition_list::conflicts(const game_object_definition& other_definition) const {
	for (auto& definition : definitions) {
		if (definition.id == other_definition.id) {
			return true;
		}
	}
	return false;
}

std::vector<game_object_definition> game_object_definition_list::of_type(game_object_type type) const {
	std::vector<game_object_definition> result;
	for (auto& definition : definitions) {
		if (definition.type == type) {
			result.push_back(definition);
		}
	}
	return result;
}

void game_object::write(no::io_stream& stream) const {
	stream.write<int32_t>(definition_id);
	stream.write<int32_t>(instance_id);
	stream.write(transform);
}

void game_object::read(no::io_stream& stream) {
	definition_id = stream.read<int32_t>();
	instance_id = stream.read<int32_t>();
	transform = stream.read<no::transform3>();
}

const game_object_definition& game_object::definition() const {
	return object_definitions().get(definition_id);
}

no::vector2i game_object::tile() const {
	return world_position_to_tile_index(transform.position.x, transform.position.z);
}

std::ostream& operator<<(std::ostream& out, game_object_type type) {
	switch (type) {
	case game_object_type::decoration: return out << "Decoration";
	case game_object_type::character: return out << "Character";
	case game_object_type::item_spawn: return out << "Item spawn";
	case game_object_type::interactive: return out << "Interactive";
	default: return out << "Unknown";
	}
}
