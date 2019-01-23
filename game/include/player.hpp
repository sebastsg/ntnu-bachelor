#pragma once

#include "object.hpp"
#include "event.hpp"

enum class equipment_slot { left_hand, right_hand, body, legs, head, feet, neck, ring, back };

class player_object : public game_object {
public:

	struct equip_event {
		equipment_slot slot;
		int item_id = -1;
	};

	struct {
		no::message_event<equip_event> equip;
	} events;

	int player_id = 0;

	player_object(world_state& world);

	void update() override;

	bool is_moving() const;
	void start_movement_to(int x, int z);

private:

	void move_towards_target();

	int target_x = -1;
	int target_z = -1;

	bool moving = false;

};
