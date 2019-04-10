#include "item.hpp"
#include "io.hpp"
#include "loop.hpp"
#include "assets.hpp"

namespace global {
static item_definition_list item_definitions;
}

item_definition_list& item_definitions() {
	return global::item_definitions;
}

item_stack_size stack_size_for_stack(long long stack) {
	if (stack > 100000) {
		return item_stack_size::large;
	}
	if (stack > 1000) {
		return item_stack_size::medium;
	}
	if (stack > 100) {
		return item_stack_size::small;
	}
	return item_stack_size::tiny;
}

void item_definition::write(no::io_stream& stream) const {
	stream.write((int32_t)type);
	stream.write((int32_t)slot);
	stream.write((int32_t)equipment);
	stream.write<uint8_t>(attachment ? 1 : 0);
	stream.write((int64_t)max_stack);
	stream.write((int32_t)id);
	stream.write(uv);
	stream.write(uv_tiny_stack);
	stream.write(uv_small_stack);
	stream.write(uv_medium_stack);
	stream.write(uv_large_stack);
	stream.write(name);
	stream.write(model);
}

void item_definition::read(no::io_stream& stream) {
	type = (item_type)stream.read<int32_t>();
	slot = (equipment_slot)stream.read<int32_t>();
	equipment = (equipment_type)stream.read<int32_t>();
	attachment = stream.read<uint8_t>() != 0;
	max_stack = stream.read<int64_t>();
	id = stream.read<int32_t>();
	uv = stream.read<no::vector2f>();
	uv_tiny_stack = stream.read<no::vector2f>();
	uv_small_stack = stream.read<no::vector2f>();
	uv_medium_stack = stream.read<no::vector2f>();
	uv_large_stack = stream.read<no::vector2f>();
	name = stream.read<std::string>();
	model = stream.read<std::string>();
}

no::vector2f item_instance::uv_for_stack() const {
	switch (stack_size_for_stack(stack)) {
	case item_stack_size::tiny: return definition().uv_tiny_stack;
	case item_stack_size::small: return definition().uv_small_stack;
	case item_stack_size::medium: return definition().uv_medium_stack;
	case item_stack_size::large: return definition().uv_large_stack;
	default: return definition().uv;
	}
}

const item_definition& item_instance::definition() const {
	return global::item_definitions.get(definition_id);
}

item_definition_list::item_definition_list() {
	invalid.name = "Invalid item";
	no::post_configure_event().listen([this] {
		load(no::asset_path("items.data"));
	});
}

void item_definition_list::save(const std::string& path) const {
	no::io_stream stream;
	stream.write((int32_t)definitions.size());
	for (auto& definition : definitions) {
		definition.write(stream);
	}
	no::file::write(path, stream);
}

void item_definition_list::load(const std::string& path) {
	definitions.clear();
	no::io_stream stream;
	no::file::read(path, stream);
	if (stream.write_index() == 0) {
		return;
	}
	int32_t count = stream.read<int32_t>();
	for (int32_t i = 0; i < count; i++) {
		item_definition definition;
		definition.read(stream);
		definitions.push_back(definition);
	}
}

item_definition& item_definition_list::get(int id) {
	if (id >= (int)definitions.size() || id < 0) {
		return invalid;
	}
	return definitions[(size_t)id];
}

std::vector<item_definition> item_definition_list::of_type(item_type type) const {
	std::vector<item_definition> result;
	for (auto& definition : definitions) {
		if (definition.type == type) {
			result.push_back(definition);
		}
	}
	return result;
}

int item_definition_list::count() const {
	return (int)definitions.size();
}

bool item_definition_list::add(const item_definition& definition) {
	if (!conflicts(definition)) {
		definitions.push_back(definition);
		return true;
	}
	return false;
}

bool item_definition_list::conflicts(const item_definition& other_definition) const {
	for (auto& definition : definitions) {
		if (definition.id == other_definition.id) {
			return true;
		}
	}
	return false;
}

item_instance inventory_container::get(no::vector2i slot) const {
	return items[slot.y * columns + slot.x];
}

void inventory_container::add_from(item_instance& other_item) {
	if (other_item.stack <= 0) {
		other_item = {};
		return;
	}
	no::vector2i slot;
	for (auto& my_item : items) {
		if (my_item.definition_id == -1 || my_item.stack < 1) {
			my_item.definition_id = other_item.definition_id;
			my_item.stack = other_item.stack;
			other_item.stack = 0;
			other_item.definition_id = -1;
			events.change.emit(slot);
			break;
		} else if (my_item.definition_id == other_item.definition_id) {
			long long can_hold = item_definitions().get(my_item.definition_id).max_stack - my_item.stack;
			if (can_hold >= other_item.stack) {
				my_item.stack += other_item.stack;
				other_item = {};
				events.change.emit(slot);
				break;
			} else {
				my_item.stack += can_hold;
				other_item.stack -= can_hold;
				events.change.emit(slot);
			}
		}
		slot.x++;
		if (slot.x == columns) {
			slot.x = 0;
			slot.y++;
		}
	}
}

void inventory_container::remove_to(long long stack, item_instance& other_item) {
	if (stack <= 0) {
		return;
	}
	long long remaining = stack;
	no::vector2i slot;
	for (auto& item : items) {
		if (item.definition_id == other_item.definition_id) {
			if (item.stack > remaining) {
				item.stack -= remaining;
				other_item.stack += remaining;
				remaining = 0;
				events.change.emit(slot);
				break;
			} else {
				other_item.stack += item.stack;
				remaining -= item.stack;
				item = {};
				events.change.emit(slot);
			}
		}
		slot.x++;
		if (slot.x == columns) {
			slot.x = 0;
			slot.y++;
		}
	}
}

long long inventory_container::take_all(int id) {
	long long total = 0;
	no::vector2i slot;
	for (auto& item : items) {
		if (item.definition_id == id) {
			total += item.stack;
			item = {};
			events.change.emit(slot);
		}
		slot.x++;
		if (slot.x == columns) {
			slot.x = 0;
			slot.y++;
		}
	}
	return total;
}

void inventory_container::clear() {
	for (auto& item : items) {
		item = {};
	}
}

long long inventory_container::can_hold_more(int id) const {
	long long can_hold = 0;
	for (auto& item : items) {
		if (item.definition_id == -1) {
			can_hold += item_definitions().get(id).max_stack;
		} else if (item.definition_id == id) {
			can_hold += item_definitions().get(id).max_stack - item.stack;
		}
	}
	return can_hold;
}

void item_instance::write(no::io_stream& stream) const {
	stream.write<int64_t>(definition_id);
	stream.write<int64_t>(stack);
}

void item_instance::read(no::io_stream& stream) {
	definition_id = stream.read<int64_t>();
	stack = stream.read<int64_t>();
}

void inventory_container::write(no::io_stream& stream) const {
	stream.write<int32_t>(columns);
	stream.write<int32_t>(rows);
	for (auto& item : items) {
		item.write(stream);
	}
}

void inventory_container::read(no::io_stream& stream) {
	stream.read<no::vector2i>(); // might be useful later
	for (int i = 0; i < slots; i++) {
		items[i].read(stream);
	}
}

item_instance equipment_container::get(equipment_slot slot) const {
	return items[(size_t)slot];
}

void equipment_container::add_from(item_instance& other_item) {
	if (other_item.stack <= 0 || other_item.definition_id < 0) {
		other_item = {};
		return;
	}
	int slot = (int)other_item.definition().slot;
	if (items[slot].definition_id != other_item.definition_id && items[slot].definition_id != -1) {
		return;
	}
	if (items[slot].definition_id == -1 || items[slot].stack < 1) {
		items[slot].definition_id = other_item.definition_id;
		items[slot].stack = other_item.stack;
		other_item.stack = 0;
		other_item.definition_id = -1;
	} else {
		long long can_hold = items[slot].definition().max_stack - items[slot].stack;
		if (can_hold >= other_item.stack) {
			items[slot].stack += other_item.stack;
			other_item = {};
		} else {
			items[slot].stack += can_hold;
			other_item.stack -= can_hold;
		}
	}
	events.change.emit(slot);
}

void equipment_container::remove_to(long long stack, item_instance& other_item) {
	if (stack <= 0 || other_item.definition_id < 0) {
		return;
	}
	int slot = (int)other_item.definition().slot;
	if (items[slot].definition_id != other_item.definition_id) {
		return;
	}
	if (items[slot].stack > stack) {
		items[slot].stack -= stack;
		other_item.stack += stack;
	} else {
		other_item.stack += items[slot].stack;
		items[slot] = {};
	}
	events.change.emit((equipment_slot)slot);
}

long long equipment_container::take_all(int id) {
	const auto& item = item_definitions().get(id);
	if (item.type != item_type::equipment) {
		return 0;
	}
	int slot = (int)item.slot;
	long long stack = items[slot].stack;
	items[slot] = {};
	events.change.emit(item.slot);
	return stack;
}

void equipment_container::clear() {
	for (auto& item : items) {
		item = {};
	}
}

long long equipment_container::can_hold_more(int id) const {
	long long can_hold = 0;
	for (auto& item : items) {
		if (item.definition_id == -1) {
			can_hold += item_definitions().get(id).max_stack;
		} else if (item.definition_id == id) {
			can_hold += item_definitions().get(id).max_stack - item.stack;
		}
	}
	return can_hold;
}

void equipment_container::write(no::io_stream& stream) const {
	for (auto& item : items) {
		item.write(stream);
	}
}

void equipment_container::read(no::io_stream& stream) {
	for (int i = 0; i < (int)equipment_slot::total_slots; i++) {
		items[i].read(stream);
	}
}

std::ostream& operator<<(std::ostream& out, item_type type) {
	switch (type) {
	case item_type::other: return out << "Other";
	case item_type::equipment: return out << "Equipment";
	case item_type::consumable: return out << "Consumable";
	default: return out << "Unknown";
	}
}

std::ostream& operator<<(std::ostream& out, equipment_slot slot) {
	switch (slot) {
	case equipment_slot::none: return out << "None";
	case equipment_slot::left_hand: return out << "Left hand";
	case equipment_slot::right_hand: return out << "Right hand";
	case equipment_slot::body: return out << "Body";
	case equipment_slot::legs: return out << "Legs";
	case equipment_slot::head: return out << "Head";
	case equipment_slot::feet: return out << "Feet";
	case equipment_slot::neck: return out << "Neck";
	case equipment_slot::ring: return out << "Ring";
	case equipment_slot::back: return out << "Back";
	default: return out << "Unknown";
	}
}

std::ostream& operator<<(std::ostream& out, equipment_type type) {
	switch (type) {
	case equipment_type::none: return out << "None";
	case equipment_type::sword: return out << "Sword";
	case equipment_type::axe: return out << "Axe";
	case equipment_type::spear: return out << "Spear";
	case equipment_type::shield: return out << "Shield";
	case equipment_type::pants: return out << "Pants";
	case equipment_type::tool: return out << "Tool";
	case equipment_type::shirt: return out << "Shirt";
	default: return out << "Unknown";
	}
}
