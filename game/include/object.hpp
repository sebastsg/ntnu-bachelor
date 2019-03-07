#pragma once

#include "transform.hpp"
#include "io.hpp"
#include "event.hpp"

#include <functional>

class world_state;
class game_object;

namespace directions {

const float north_west = 225.0f;
const float north_east = 135.0f;
const float south_west = 315.0f;
const float south_east = 45.0f;
const float west = 270.0f;
const float east = 90.0f;
const float south = 0.0f;
const float north = 180.0f;

}

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
	int dialogue_id = -1;
	std::string description;
};

class game_object_definition_list {
public:

	game_object_definition_list();

	void save(const std::string& path) const;
	void load(const std::string& path);

	game_object_definition& get(long long id);
	const game_object_definition& get(long long id) const;
	int count() const;

	bool add(const game_object_definition& definition);
	bool conflicts(const game_object_definition& definition) const;
	game_object* construct(int definition_id) const;

private:

	std::vector<game_object_definition> definitions;
	std::vector<std::function<game_object*()>> constructors;
	game_object_definition invalid;

};

game_object_definition_list& object_definitions();

class game_object {
public:

	friend class world_objects;

	no::transform3 transform;

	virtual ~game_object() = default;

	virtual void update() = 0;
	virtual void write(no::io_stream& stream) const;
	virtual void read(no::io_stream& stream);

	float x() const;
	float y() const;
	float z() const;
	int id() const;
	const game_object_definition& definition() const;
	no::vector2i tile() const;

	void change_id(int id);
	bool change_definition(int id);
	void change_world(world_state& world);

	no::io_stream serialized() const;

protected:

	world_state* world = nullptr;
	int definition_id = -1;
	int instance_id = -1;

};

class world_objects {
public:

	struct add_event {
		game_object* object = nullptr;
	};

	struct remove_event {
		game_object* object = nullptr;
	};
	
	struct {
		no::message_event<add_event> add;
		no::message_event<remove_event> remove;
	} events;

	world_objects(world_state& world);
	world_objects(const world_objects&) = delete;
	world_objects(world_objects&&);
	~world_objects();

	world_objects& operator=(const world_objects&) = delete;
	world_objects& operator=(world_objects&&) = delete;

	game_object* add(int definition_id);
	game_object* add(no::io_stream& stream);
	game_object* find(int instance_id);
	void remove(int instance_id);
	void for_each(const std::function<void(game_object*)>& handler);

	void update();

	void load(const std::string& path);
	void save(const std::string& path) const;

	int next_dynamic_id(); // for objects created by server
	int next_static_id(); // for objects in the world assets

private:

	static constexpr int dynamic_id_start = 0;
	static constexpr int static_id_start = 100000;

	int next_id_from(int from_id);

	world_state& world;
	std::vector<game_object*> objects;

};

std::ostream& operator<<(std::ostream& out, game_object_type type);
