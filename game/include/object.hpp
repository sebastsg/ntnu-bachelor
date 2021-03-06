#pragma once

#include "transform.hpp"
#include "io.hpp"
#include "event.hpp"

#include <functional>

class world_state;

enum class game_object_type {
	invalid,
	decoration,
	character,
	item_spawn,
	interactive,
	total_types
};

struct game_object_definition {
	int id = -1;
	game_object_type type = game_object_type::decoration;
	std::string name;
	std::string model;
	no::transform3 bounding_box;
	struct {
		int dialogue = -1;
		int killed = -1;
	} script_id;
	std::string description;
};

struct game_object {

	no::transform3 transform;
	int definition_id = -1;
	int instance_id = -1;
	bool pickable = true;

	void write(no::io_stream& stream) const;
	void read(no::io_stream& stream);

	const game_object_definition& definition() const;
	no::vector2i tile() const;

};

class game_object_definition_list {
public:

	game_object_definition_list();

	void save(const std::string& path) const;
	void load(const std::string& path);

	game_object_definition& get(int id);
	const game_object_definition& get(int id) const;
	int count() const;

	bool add(const game_object_definition& definition);
	bool conflicts(const game_object_definition& definition) const;
	std::vector<game_object_definition> of_type(game_object_type type) const;

private:

	std::vector<game_object_definition> definitions;
	game_object_definition invalid;

};

template<typename T>
no::io_stream serialized_object(const T& object) {
	no::io_stream stream;
	object.write(stream);
	return stream;
}

game_object_definition_list& object_definitions();
const game_object_definition& object_definition(int id);

std::ostream& operator<<(std::ostream& out, game_object_type type);
