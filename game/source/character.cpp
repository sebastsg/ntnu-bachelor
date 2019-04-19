#include "character.hpp"
#include "world.hpp"
#include "pathfinding.hpp"

std::ostream& operator<<(std::ostream& out, stat_type stat) {
	switch (stat) {
	case stat_type::health: return out << "Health";
	case stat_type::stamina: return out << "Stamina";
	case stat_type::sword: return out << "Swordsmanship";
	case stat_type::defensive: return out << "Defensive";
	case stat_type::axe: return out << "Axe";
	case stat_type::spear: return out << "Spear";
	case stat_type::fishing: return out << "Fishing";
	default: return out << "Unknown";
	}
}

void character_stat::set_experience(long long experience) {
	current_experience = 0;
	real_level = 0;
	effective_level = 0;
	add_experience(experience);
}

void character_stat::add_experience(long long experience) {
	current_experience += experience;
	while (current_experience >= experience_for_level(real_level + 1)) {
		real_level++;
		effective_level++;
	}
}

long long character_stat::experience() const {
	return current_experience;
}

long long character_stat::experience_left() const {
	return experience_for_level(real_level + 1) - current_experience;
}

long long character_stat::experience_for_level(int level) const {
	return level * level * 97;
}

float character_stat::progress() const {
	long long goal = experience_for_level(real_level + 1) - experience_for_level(real_level);
	return 1.0f - (float)experience_left() / (float)goal;
}

void character_stat::add_effective(int amount) {
	effective_level = std::max(0, std::min(real_level, effective_level + amount));
}

int character_stat::level_for_experience(long long experience) const {
	int level = 1;
	while (experience >= experience_for_level(level + 1)) {
		level++;
	}
	return level;
}

int character_stat::real() const {
	return real_level;
}

int character_stat::effective() const {
	return effective_level;
}

void character_stat::write(no::io_stream& stream) const {
	stream.write<int64_t>(current_experience);
}

void character_stat::read(no::io_stream& stream) {
	current_experience = stream.read<int64_t>();
	real_level = level_for_experience(current_experience);
	effective_level = real_level;
}


character_object::character_object(int object_id) : object_id{ object_id } {
	last_combat_event_timer.start();
	alive_timer.start();
	events.attack.listen([this] {
		last_combat_event_timer.start();
	});
	events.defend.listen([this] {
		last_combat_event_timer.start();
	});
	events.start_fishing.listen([this] {
		fishing = true;
	});
	events.stop_fishing.listen([this] {
		fishing = false;
	});
}

void character_object::update(world_state& world, game_object& object) {
	object.transform.position.y = world.terrain.average_elevation_at(object.tile());
	while (!target_path.empty() && target_path.back() < 0) {
		target_path.pop_back();
	}
	int path_count = (int)target_path.size();
	bool moved = move_towards_target(object.transform, target_path, speed());
	if (moved != moved_last_frame) {
		if (moved) {
			events.move.emit(running);
		} else {
			events.idle.emit();
		}
	}
	moved_last_frame = moved;
	if (path_count > (int)target_path.size()) {
		tiles_moved++;
	}
}

void character_object::start_path_movement(const std::vector<no::vector2i>& path) {
	target_path = path;
	tiles_moved = 0;
	walk_around_timer.start();
}

void character_object::write(no::io_stream& stream) const {
	inventory.write(stream);
	equipment.write(stream);
	stream.write((int32_t)stat_type::total);
	for (auto& stat : stats) {
		stat.write(stream);
	}
	stream.write<uint8_t>(walking_around ? 1 : 0);
	stream.write(walking_around_center);
	stream.write(name);
}

void character_object::read(no::io_stream& stream) {
	inventory.read(stream);
	equipment.read(stream);
	int count = stream.read<int32_t>();
	for (int i = 0; i < count; i++) {
		stats[i].read(stream);
	}
	walking_around = (stream.read<uint8_t>() != 0);
	walking_around_center = stream.read<no::vector2i>();
	name = stream.read<std::string>();
}

void character_object::equip_from_inventory(no::vector2i slot) {
	item_instance item = inventory.get(slot);
	item.stack = 0;
	inventory.remove_to(inventory.get(slot).stack, item);
	events.equip.emit(item);
	equipment.add_from(item);
}

void character_object::unequip_to_inventory(equipment_slot slot) {
	item_instance item = equipment.get(slot);
	item.stack = 0;
	equipment.remove_to(equipment.get(slot).stack, item);
	inventory.add_from(item);
	events.unequip.emit(item.definition().slot);
}

void character_object::equip(item_instance item) {
	equipment.add_from(item);
	events.equip.emit(item);
}

void character_object::unequip(equipment_slot slot) {
	equipment.items[(int)slot] = {};
	events.unequip.emit(slot);
}

void character_object::consume_from_inventory(no::vector2i slot) {
	item_instance item = inventory.get(slot);
	item.stack = 0;
	inventory.remove_to(inventory.get(slot).stack, item);
	stat(stat_type::health).add_effective(5);
	equipment.add_from(item);
}

bool character_object::is_moving() const {
	return target_path.size() > 0;
}

bool character_object::in_combat() const {
	return last_combat_event_timer.seconds() < 5 && alive_timer.seconds() > 5;
}

bool character_object::is_fishing() const {
	return fishing;
}

float character_object::speed() const {
	return running ? run_speed : walk_speed;
}

character_stat& character_object::stat(stat_type stat) {
	return stats[(size_t)stat];
}

const character_stat& character_object::stat(stat_type stat) const {
	return stats[(size_t)stat];
}

void character_object::add_combat_experience(int damage) {
	auto& right = equipment.get(equipment_slot::right_hand).definition();
	auto& left = equipment.get(equipment_slot::left_hand).definition();
	const long long attack_exp = damage * 12LL;
	const long long defend_exp = damage * 9LL;
	const long long health_exp = damage * 6LL;
	switch (right.equipment) {
	case equipment_type::axe:
		stat(stat_type::axe).add_experience(attack_exp);
		break;
	case equipment_type::sword:
		stat(stat_type::sword).add_experience(attack_exp);
		break;
	case equipment_type::spear:
		stat(stat_type::spear).add_experience(attack_exp);
		break;
	}
	if (left.equipment == equipment_type::shield) {
		stat(stat_type::defensive).add_experience(defend_exp);
	}
	stat(stat_type::health).add_experience(health_exp);
}

int character_object::combat_level() const {
	float base = 0.3f * (float)(stat(stat_type::defensive).real() + stat(stat_type::health).real());
	float melee = 0.5f * (float)(stat(stat_type::axe).real() + stat(stat_type::sword).real() + stat(stat_type::spear).real());
	return (int)(base + melee);
}
