#pragma once

#include "world.hpp"
#include "combat.hpp"

class server_state;

class server_world : public world_state {
public:

	struct kill_event {
		int attacker_id = -1;
		int target_id = -1;
	};

	struct move_event {
		int object_id = -1;
		std::vector<no::vector2i> path;
	};

	struct rotate_event {
		int object_id = -1;
		no::vector3f rotation;
	};

	struct {
		no::event_message_queue<kill_event> kill;
		no::event_message_queue<move_event> move;
		no::event_message_queue<rotate_event> rotate;
	} events;

	combat_system combat;

	struct fishing_state {
		no::timer fished_for;
		no::timer last_progress;
		int client_index = -1;
		int player_instance_id = -1;
		no::vector2i bait_tile;
		bool finished = false;
	};

	std::vector<fishing_state> fishers;

	server_world(server_state& server, const std::string& name);

	void update() override;

private:

	void update_fishing();
	void update_random_walk_movement(character_object& character, game_object& object);

	server_state& server;
	no::random_number_generator random;

};
