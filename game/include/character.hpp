#pragma once

#include "item.hpp"
#include "event.hpp"
#include "object.hpp"
#include "timer.hpp"

enum class stat_type { health, stamina, sword, defensive, axe, spear, fishing, total };

std::ostream& operator<<(std::ostream& out, stat_type stat);

class character_stat {
public:

	void set_experience(long long experience);
	void add_experience(long long experience);
	long long experience() const;
	long long experience_left() const;
	long long experience_for_level(int level) const;
	float progress() const;
	
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

	float run_speed = 0.05f;
	float walk_speed = 0.03f;

	int object_id = -1;
	bool walking_around = false;
	bool running = false;
	std::string name; // to override definition name

	struct {
		no::message_event<item_instance> equip;
		no::message_event<equipment_slot> unequip;
		no::signal_event attack;
		no::signal_event defend;
		no::signal_event idle;
		no::message_event<bool> move;
		no::signal_event start_fishing;
		no::signal_event stop_fishing;
	} events;

	inventory_container inventory;
	equipment_container equipment;

	character_object(int object_id);

	void update(world_state& world, game_object& object);
	void start_path_movement(const std::vector<no::vector2i>& path);

	void write(no::io_stream& stream) const;
	void read(no::io_stream& stream);

	void equip_from_inventory(no::vector2i slot);
	void unequip_to_inventory(equipment_slot slot);
	void equip(item_instance item);
	void unequip(equipment_slot slot);
	void consume_from_inventory(no::vector2i slot);

	bool is_moving() const;
	bool in_combat() const;
	bool is_fishing() const;
	float speed() const;

	character_stat& stat(stat_type stat);
	const character_stat& stat(stat_type stat) const;
	void add_combat_experience(int damage);
	int combat_level() const;

	no::vector2i walking_around_center;
	no::vector2i bait_tile;
	std::vector<no::vector2i> target_path;
	int tiles_moved = 0;
	bool moved_last_frame = false;
	character_stat stats[(size_t)stat_type::total];
	no::timer walk_around_timer;
	no::timer last_combat_event_timer;
	no::timer alive_timer;
	bool fishing = false;

};
