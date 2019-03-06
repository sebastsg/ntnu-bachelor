#pragma once

#include "object.hpp"
#include "item.hpp"
#include "event.hpp"

class ranged_value {
public:

	ranged_value() = default;
	ranged_value(int value, int min, int max);

	int add(int amount);
	float normalized() const;
	int value() const;
	int min() const;
	int max() const;

	void write(no::io_stream& stream) const;
	void read(no::io_stream& stream);

private:
	
	int current_value = 0;
	int min_value = 0;
	int max_value = 1;

};

class character_object : public game_object {
public:

	struct equip_event {
		item_instance item;
	};

	struct unequip_event {
		equipment_slot slot = equipment_slot::none;
	};

	struct {
		no::message_event<equip_event> equip;
		no::message_event<unequip_event> unequip;
		no::signal_event attack;
		no::signal_event defend;
	} events;

	item_container inventory;
	item_container equipment;
	ranged_value health;

	void update() override;
	void write(no::io_stream& stream) const override;
	void read(no::io_stream& stream) override;

	void equip_from_inventory(no::vector2i slot);
	void unequip_to_inventory(no::vector2i slot);
	void equip(item_instance item);

	bool is_moving() const;
	void start_path_movement(const std::vector<no::vector2i>& path);

private:

	void move_towards_target();

	std::vector<no::vector2i> target_path;
	bool moving = false;

};
