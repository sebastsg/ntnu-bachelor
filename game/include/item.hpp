#pragma once

#include "io.hpp"
#include "math.hpp"
#include "event.hpp"

#include <unordered_map>

enum class item_type {
	other,
	equipment,
	consumable,
	total_types
};

enum class equipment_slot {
	none,
	left_hand,
	right_hand,
	body,
	legs,
	head,
	feet,
	neck,
	ring,
	back,
	total_slots
};

struct item_definition {
	int id = -1;
	long long max_stack = 1;
	item_type type = item_type::other;
	equipment_slot slot = equipment_slot::none;
	std::string name;
	no::vector2f uv;
	std::string model;
};

struct item_instance {

	int definition_id = -1;
	long long stack = 0;

	const item_definition& definition() const;

};

class item_definition_list {
public:

	item_definition_list();

	void save(const std::string& path) const;
	void load(const std::string& path);

	item_definition& get(int id);
	std::vector<item_definition> of_type(item_type type) const;
	int count() const;

	bool add(const item_definition& definition);
	bool conflicts(const item_definition& definition) const;

private:

	std::vector<item_definition> definitions;
	item_definition invalid;

};

item_definition_list& item_definitions();

struct inventory_container {
	
	static const int columns = 4;
	static const int rows = 4;
	static const int slots = columns * rows;

	struct {
		no::message_event<no::vector2i> change;
	} events;

	item_instance items[slots];

	item_instance get(no::vector2i slot) const;
	void add_from(item_instance& item);
	void remove_to(long long stack, item_instance& item);
	long long take_all(int id);
	void clear();
	long long can_hold_more(int id) const;

	void write(no::io_stream& stream) const;
	void read(no::io_stream& stream);

};

struct equipment_container {

	struct {
		no::message_event<equipment_slot> change;
	} events;

	item_instance items[(size_t)equipment_slot::total_slots];

	item_instance get(equipment_slot slot) const;
	void add_from(item_instance& item);
	void remove_to(long long stack, item_instance& item);
	long long take_all(int id);
	void clear();
	long long can_hold_more(int id) const;

	void write(no::io_stream& stream) const;
	void read(no::io_stream& stream);

};

std::ostream& operator<<(std::ostream& out, equipment_slot slot);
std::ostream& operator<<(std::ostream& out, item_type type);
