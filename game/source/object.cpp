#include "object.hpp"
#include "world.hpp"

std::ostream& operator<<(std::ostream& out, game_object_type type) {
	switch (type) {
	case game_object_type::decoration: return out << "Decoration";
	case game_object_type::character: return out << "Character";
	case game_object_type::item_spawn: return out << "Item spawn";
	case game_object_type::interactive: return out << "Interactive";
	default: return out << "Unknown";
	}
}

game_object_definition_list::game_object_definition_list() {
	invalid.name = "Invalid object";
}

void game_object_definition_list::save(const std::string& path) const {
	no::io_stream stream;
	stream.write((int32_t)definitions.size());
	for (auto& definition : definitions) {
		stream.write((int32_t)definition.type);
		stream.write((int64_t)definition.id);
		stream.write(definition.name);
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
		definitions.push_back(definition);
	}
}

game_object_definition& game_object_definition_list::get(long long id) {
	if (id >= definitions.size() || id < 0) {
		return invalid;
	}
	return definitions[(size_t)id];
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

game_object::game_object(world_state& world) : world(world) {
	instance_id = world.next_object_id();
}

void game_object::write(no::io_stream& stream) const {
	stream.write(transform);
	stream.write((int32_t)definition_id);
	stream.write((int32_t)instance_id);
}

void game_object::read(no::io_stream& stream) {
	transform = stream.read<no::transform>();
	definition_id = stream.read<int32_t>();
	instance_id = stream.read<int32_t>();
}

float game_object::x() const {
	return transform.position.x;
}

float game_object::y() const {
	return transform.position.y;
}

float game_object::z() const {
	return transform.position.z;
}

int game_object::id() const {
	return instance_id;
}

int game_object::definition() const {
	return definition_id;
}

no::vector2i game_object::tile() const {
	return world.world_position_to_tile_index(x(), z());
}

decoration_object::decoration_object(world_state& world) : game_object(world) {
	transform.scale = 0.5f;
}

void decoration_object::update() {

}

void decoration_object::write(no::io_stream& stream) const {
	game_object::write(stream);
	stream.write(model);
	stream.write(texture);
}

void decoration_object::read(no::io_stream& stream) {
	game_object::read(stream);
	model = stream.read<std::string>();
	texture = stream.read<std::string>();
}
