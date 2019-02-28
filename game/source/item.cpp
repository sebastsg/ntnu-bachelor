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
		stream.write((int32_t)definition.type);
		stream.write((int32_t)definition.slot);
		stream.write((int64_t)definition.max_stack);
		stream.write((int64_t)definition.id);
		stream.write(definition.uv);
		stream.write(definition.name);
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
		definition.type = (item_type)stream.read<int32_t>();
		definition.slot = (equipment_slot)stream.read<int32_t>();
		definition.max_stack = stream.read<int64_t>();
		definition.id = stream.read<int64_t>();
		definition.uv = stream.read<no::vector2f>();
		definition.name = stream.read<std::string>();
		definitions.push_back(definition);
	}
}

item_definition& item_definition_list::get(long long id) {
	if (id >= definitions.size() || id < 0) {
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

void item_container::resize(no::vector2i new_size) {
	size = new_size;
	items.resize(size.x * size.y);
}

item_instance item_container::at(no::vector2i slot) const {
	int index = slot.y * size.x + slot.x;
	if (index < 0 || index >= count()) {
		return {};
	}
	return items[index];
}

void item_container::for_each(const std::function<void(no::vector2i, const item_instance&)>& handler) {
	no::vector2i slot;
	for (auto& item : items) {
		if (item.definition_id != -1) {
			handler(slot, item);
		}
		slot.x++;
		if (slot.x == size.x) {
			slot.x = 0;
			slot.y++;
		}
	}
}

void item_container::add_from(item_instance& other_item) {
	if (other_item.stack <= 0) {
		other_item = {};
		return;
	}
	no::vector2i slot;
	for (auto& my_item : items) {
		if (my_item.definition_id == -1 || my_item.stack < 1) {
			my_item.definition_id = other_item.definition_id;
			my_item.stack = other_item.stack;
			events.add.emit(other_item, slot);
			other_item.stack = 0;
			other_item.definition_id = -1;
			break;
		} else if (my_item.definition_id == other_item.definition_id) {
			long long can_hold = item_definitions().get(my_item.definition_id).max_stack - my_item.stack;
			if (can_hold >= other_item.stack) {
				my_item.stack += other_item.stack;
				events.add.emit(other_item, slot);
				other_item = {};
				break;
			} else {
				my_item.stack += can_hold;
				other_item.stack -= can_hold;
				events.add.emit(item_instance{ other_item.definition_id, can_hold }, slot);
			}
		}
		slot.x++;
		if (slot.x == size.x) {
			slot.x = 0;
			slot.y++;
		}
	}
}

void item_container::remove_to(long long stack, item_instance& other_item) {
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
				events.remove.emit(item, slot);
				break;
			} else {
				remaining -= item.stack;
				remove_event event;
				event.item = item;
				event.slot = slot;
				item = {};
				events.remove.emit(event);
			}
		}
		slot.x++;
		if (slot.x == size.x) {
			slot.x = 0;
			slot.y++;
		}
	}
}

long long item_container::take_all(long long id) {
	long long total = 0;
	no::vector2i slot;
	for (auto& item : items) {
		if (item.definition_id == id) {
			total += item.stack;
			remove_event event;
			event.item = item;
			event.slot = slot;
			item = {};
			events.remove.emit(event);
		}
		slot.x++;
		if (slot.x == size.x) {
			slot.x = 0;
			slot.y++;
		}
	}
	return total;
}

void item_container::clear() {
	for (auto& item : items) {
		item = {};
	}
}

long long item_container::can_hold_more(long long id) const {
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

void item_container::write(no::io_stream& stream) const {
	stream.write(size);
	for (auto& item : items) {
		stream.write<int64_t>(item.definition_id);
		stream.write<int64_t>(item.stack);
	}
}

void item_container::read(no::io_stream& stream) {
	size = stream.read<no::vector2i>();
	for (int i = 0; i < count(); i++) {
		auto& item = items.emplace_back();
		item.definition_id = stream.read<int64_t>();
		item.stack = stream.read<int64_t>();
	}
}

int item_container::rows() const {
	return size.y;
}

int item_container::columns() const {
	return size.x;
}

int item_container::count() const {
	return size.x * size.y;
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

std::ostream& operator<<(std::ostream& out, item_type type) {
	switch (type) {
	case item_type::other: return out << "Other";
	case item_type::equipment: return out << "Equipment";
	case item_type::consumable: return out << "Consumable";
	default: return out << "Unknown";
	}
}
