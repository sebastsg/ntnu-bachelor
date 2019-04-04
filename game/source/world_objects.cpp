#include "world_objects.hpp"
#include "world.hpp"
#include "character.hpp"

world_objects::world_objects(world_state& world) : world(world) {

}

int world_objects::add(int definition_id) {
	game_object object;
	object.definition_id = definition_id;
	for (int i = 0; i < (int)objects.size(); i++) {
		if (objects[i].instance_id == -1) {
			object.instance_id = i;
			objects[i] = object;
			break;
		}
	}
	if (object.instance_id == -1) {
		object.instance_id = (int)objects.size();
		objects.push_back(object);
	}
	if (object.definition().type == game_object_type::character) {
		characters.emplace_back(object.instance_id);
	}
	events.add.emit(object);
	return object.instance_id;
}

int world_objects::add(no::io_stream& stream) {
	game_object object;
	object.read(stream);
	if (object.instance_id == -1) {
		object.instance_id = (int)objects.size();
		objects.push_back(object);
	} else {
		while (object.instance_id >= (int)objects.size()) {
			objects.emplace_back();
		}
		for (int i = 0; i < (int)objects.size(); i++) {
			if (object.instance_id == i) {
				objects[i] = object;
				break;
			}
		}
	}
	if (object.definition().type == game_object_type::character) {
		characters.emplace_back(object.instance_id).read(stream);
	}
	events.add.emit(object);
	return object.instance_id;
}

game_object& world_objects::object(int instance_id) {
	return objects[instance_id];
}

const game_object& world_objects::object(int instance_id) const {
	return objects[instance_id];
}

character_object* world_objects::character(int instance_id) {
	for (auto& character : characters) {
		if (character.object_id == instance_id) {
			return &character;
		}
	}
	return nullptr;
}

const character_object* world_objects::character(int instance_id) const {
	for (auto& character : characters) {
		if (character.object_id == instance_id) {
			return &character;
		}
	}
	return nullptr;
}

void world_objects::remove(int instance_id) {
	if (instance_id < 0 || instance_id >= (int)objects.size()) {
		return;
	}
	events.remove.emit(objects[instance_id]);
	objects[instance_id] = {};
	for (int i = 0; i < (int)characters.size(); i++) {
		if (characters[i].object_id == instance_id) {
			characters.erase(characters.begin() + i);
			break;
		}
	}
}

void world_objects::for_each(const std::function<void(game_object*)>& handler) {
	for (auto& object : objects) {
		handler(&object);
	}
}

void world_objects::for_each(const std::function<void(const game_object*)>& handler) const {
	for (const auto object : objects) {
		handler(&object);
	}
}

void world_objects::update() {
	for (auto& character : characters) {
		character.update(world, object(character.object_id));
	}
}

int world_objects::count() const {
	return (int)objects.size();
}

void world_objects::load(const std::string& path) {
	no::io_stream stream;
	no::file::read(path, stream);
	int count = stream.read<int32_t>();
	for (int i = 0; i < count; i++) {
		int definition_id = stream.read<int32_t>();
		game_object new_object;
		new_object.read(stream);
		if (new_object.instance_id == -1) {
			continue;
		}
		int instance_id = add(definition_id);
		objects[instance_id] = new_object;
		objects[instance_id].instance_id = instance_id;
		if (new_object.definition().type == game_object_type::character) {
			characters.back().read(stream); // added in add()
		}
	}
}

void world_objects::save(const std::string& path) const {
	no::io_stream stream;
	stream.write((int32_t)objects.size());
	for (auto& object : objects) {
		stream.write<int32_t>(object.definition().id);
		object.write(stream);
		if (character(object.instance_id)) {
			character(object.instance_id)->write(stream);
		}
	}
	no::file::write(path, stream);
}
