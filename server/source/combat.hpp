#pragma once

#include "event.hpp"
#include "timer.hpp"

class server_world;
class character_object;

class active_combat {
public:

	int attacker_id = -1;
	int target_id = -1;

	active_combat(int attacker_id, int target_id, server_world& world);

	bool can_hit() const;
	bool is_target_in_range() const;
	int hit();
	void next_turn();

private:

	server_world* world = nullptr;
	no::timer last_hit;

};

class combat_system {
public:

	struct hit_event {
		int attacker_id = -1;
		int target_id = -1;
		int damage = 0;
	};

	struct {
		no::message_event<hit_event> hit;
	} events;

	combat_system(server_world& world);
	combat_system(const combat_system&) = delete;
	combat_system(combat_system&&) = delete;

	~combat_system();

	combat_system& operator=(const combat_system&) = delete;
	combat_system& operator=(combat_system&&) = delete;

	void update();
	void add(int attacker_id, int target_id);

	void stop_all(int object_id);

private:

	std::vector<active_combat> combats;
	server_world& world;
	int object_remove_event = -1;

};
