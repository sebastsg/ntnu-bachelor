#pragma once

#include "item.hpp"
#include "event.hpp"
#include "object.hpp"

enum class stat_type { health, stamina, sword, defensive, axe, spear, fishing, total };

class character_stat {
public:

	void set_experience(long long experience);
	void add_experience(long long experience);
	long long experience() const;
	long long experience_left() const;
	long long experience_for_level(int level) const;
	
	void add_effective(int amount);
	int level_for_experience(long long experience) const;
	int real() const;
	int effective() const;

	void write(no::io_stream& stream) const;
	void read(no::io_stream& stream);

private:
	
	int real_level = 0;
	int effective_level = 0;
	long long current_experience = 0;

};

class character_object {
public:

	int object_id = -1;
	int follow_object_id = -1;
	int follow_distance = 1;

	struct {
		no::message_event<item_instance> equip;
		no::message_event<equipment_slot> unequip;
		no::signal_event attack;
		no::signal_event defend;
		no::message_event<bool> run;
	} events;

	item_container inventory;
	item_container equipment;

	character_object(int object_id) : object_id(object_id) {}

	void update(world_state& world, game_object& object);
	void start_path_movement(const std::vector<no::vector2i>& path);

	void write(no::io_stream& stream) const;
	void read(no::io_stream& stream);

	void equip_from_inventory(no::vector2i slot);
	void unequip_to_inventory(no::vector2i slot);
	void equip(item_instance item);

	bool is_moving() const;

	character_stat& stat(stat_type stat);

private:

	std::vector<no::vector2i> target_path;
	int tiles_moved = 0;
	bool moved_last_frame = false;
	character_stat stats[(size_t)stat_type::total];

};
