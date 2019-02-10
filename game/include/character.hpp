#pragma once

#include "object.hpp"
#include "item.hpp"
#include "event.hpp"

class character_object : public game_object {
public:

	struct equip_event {
		equipment_slot slot;
		long long item_id = -1;
	};

	item_container inventory;
	item_container equipment;

	struct {
		no::message_event<equip_event> equip;
	} events;

	void update() override;
	void write(no::io_stream& stream) const override;
	void read(no::io_stream& stream) override;

	bool is_moving() const;
	void start_movement_to(int x, int z);

private:

	void move_towards_target();

	int target_x = -1;
	int target_z = -1;

	bool moving = false;

};