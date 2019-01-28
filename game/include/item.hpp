#pragma once

#include "io.hpp"
#include "math.hpp"

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

std::ostream& operator<<(std::ostream& out, equipment_slot slot);
std::ostream& operator<<(std::ostream& out, item_type type);

struct item_definition {
	long long id = -1;
	long long max_stack = 1;
	item_type type = item_type::other;
	equipment_slot slot = equipment_slot::none;
	std::string name;
};

struct item_instance {
	long long definition_id = -1;
	long long instance_id = -1;
	long long stack = 0;
};

class item_definition_list {
public:

	item_definition_list();

	void save(const std::string& path) const;
	void load(const std::string& path);

	item_definition& get(long long id);
	int count() const;

	bool add(const item_definition& definition);
	bool conflicts(const item_definition& definition) const;

private:

	std::vector<item_definition> definitions;
	item_definition invalid;

};

class item_container {
public:

	item_container(item_definition_list& definitions, no::vector2i size);

	void add_from(item_instance& item);
	void remove_to(long long stack, item_instance& item);

	long long can_hold_more(long long id) const;

	void write(no::io_stream& stream) const;
	void read(no::io_stream& stream);

	int rows() const;
	int columns() const;
	int count() const;

private:

	item_definition_list& definitions;
	std::vector<item_instance> items;
	no::vector2i size;

};
