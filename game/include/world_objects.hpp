#pragma once

#include "character.hpp"

struct game_object;
class character_object;
class world_state;

class world_objects {
public:

	struct {
		no::message_event<game_object> add;
		no::message_event<game_object> remove;
	} events;

	world_objects(world_state& world);

	int add(int definition_id);
	int add(no::io_stream& stream);
	game_object& object(int instance_id);
	const game_object& object(int instance_id) const;
	character_object* character(int instance_id);
	const character_object* character(int instance_id) const;
	void remove(int instance_id);
	void for_each(const std::function<void(game_object*)>& handler);
	void update();
	int count() const;

	void load(const std::string& path);
	void save(const std::string& path) const;

private:

	world_state& world;
	std::vector<game_object> objects;
	std::vector<character_object> characters;

};
