#include "object.hpp"
#include "world.hpp"
#include "character.hpp"
#include "decoration.hpp"
#include "assets.hpp"
#include "loop.hpp"
#include "assets.hpp"

namespace global {
static game_object_definition_list object_definitions;
}

game_object_definition_list& object_definitions() {
	return global::object_definitions;
}

game_object_definition_list::game_object_definition_list() {
	invalid.name = "Invalid object";
	constructors.push_back([] { return nullptr; });
	constructors.push_back([] { return new decoration_object(); });
	constructors.push_back([] { return new character_object(); });
	constructors.push_back([] { return nullptr; });
	constructors.push_back([] { return nullptr; });
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
		definitions.push_back(definition);
	}
}

game_object_definition& game_object_definition_list::get(long long id) {
	if (id >= definitions.size() || id < 0) {
		return invalid;
	}
	return definitions[(size_t)id];
}

const game_object_definition& game_object_definition_list::get(long long id) const {
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

game_object* game_object_definition_list::construct(int definition_id) const {
	auto object = constructors[(int)get(definition_id).type]();
	if (object) {
		object->change_definition(definition_id);
	}
	return object;
}

void game_object::write(no::io_stream& stream) const {
	stream.write<int32_t>(definition_id);
	stream.write<int32_t>(instance_id);
	stream.write(transform);
}

void game_object::read(no::io_stream& stream) {
	definition_id = stream.read<int32_t>();
	instance_id = stream.read<int32_t>();
	transform = stream.read<no::transform>();
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

const game_object_definition& game_object::definition() const {
	return object_definitions().get(definition_id);
}

no::vector2i game_object::tile() const {
	return world->world_position_to_tile_index(x(), z());
}

void game_object::change_id(int id) {
	instance_id = id;
}

bool game_object::change_definition(int id) {
	if (id >= object_definitions().count()) {
		return false;
	}
	definition_id = id;
	return true;
}

void game_object::change_world(world_state& new_world) {
	world = &new_world;
}

no::io_stream game_object::serialized() const {
	no::io_stream stream;
	write(stream);
	return stream;
}

world_objects::world_objects(world_state& world) : world(world) {

}

world_objects::world_objects(world_objects&& that) : world(that.world) {
	std::swap(objects, that.objects);
	std::swap(events, that.events);
}

world_objects::~world_objects() {
	for (auto object : objects) {
		delete object;
	}
}

void world_objects::update() {
	for (auto& object : objects) {
		object->update();
	}
}

game_object* world_objects::add(int definition_id) {
	auto object = object_definitions().construct(definition_id);
	if (object) {
		object->change_world(world);
		objects.push_back(object);
		events.add.emit(object);
	}
	return object;
}

game_object* world_objects::add(no::io_stream& stream) {
	int definition_id = *(int32_t*)stream.at_read();
	auto object = object_definitions().construct(definition_id);
	if (object) {
		object->read(stream);
		object->change_world(world);
		objects.push_back(object);
		events.add.emit(object);
	}
	return object;
}

game_object* world_objects::find(int instance_id) {
	for (auto& object : objects) {
		if (object->id() == instance_id) {
			return object;
		}
	}
	return nullptr;
}

void world_objects::remove(int instance_id) {
	for (size_t i = 0; i < objects.size(); i++) {
		if (objects[i]->id() == instance_id) {
			events.remove.emit(objects[i]);
			delete objects[i];
			objects.erase(objects.begin() + i);
			break;
		}
	}
}

void world_objects::for_each(const std::function<void(game_object*)>& handler) {
	for (auto object : objects) {
		handler(object);
	}
}

void world_objects::load(const std::string& path) {
	no::io_stream stream;
	no::file::read(path, stream);
	int32_t count = stream.read<int32_t>();
	for (int32_t i = 0; i < count; i++) {
		int32_t definition_id = stream.read<int32_t>();
		auto object = add(definition_id);
		object->read(stream);
	}
}

void world_objects::save(const std::string& path) const {
	no::io_stream stream;
	stream.write((int32_t)objects.size());
	for (auto& object : objects) {
		stream.write<int32_t>(object->definition().id);
		object->write(stream);
	}
	no::file::write(path, stream);
}

int world_objects::next_dynamic_id() {
	return next_id_from(dynamic_id_start);
}

int world_objects::next_static_id() {
	return next_id_from(static_id_start);
}

int world_objects::next_id_from(int from_id) {
	int counter = from_id;
	while (find(counter)) {
		counter++;
	}
	return counter;
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
