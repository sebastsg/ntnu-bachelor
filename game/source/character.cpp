#include "character.hpp"
#include "world.hpp"
#include "pathfinding.hpp"

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
	bool moved = move_towards_target(object.transform, target_path);
	if (moved != moved_last_frame) {
		events.run.emit(moved);
	}
	moved_last_frame = moved;
	if (path_count > (int)target_path.size()) {
		tiles_moved++;
	}
	if (follow_object_id != -1 && (tiles_moved > 1 || target_path.empty())) {
		auto& target = world.objects.object(follow_object_id);
		no::vector2i target_tile = target_tile_at_distance(target, object, follow_distance);
		bool new_path_x = std::abs(object.tile().x - target_tile.x) > follow_distance;
		bool new_path_y = std::abs(object.tile().y - target_tile.y) > follow_distance;
		if (new_path_x || new_path_y) {
			start_path_movement(world.path_between(object.tile(), target_tile));
		} else {
			float new_angle = angle_to_goal(object.tile(), target_tile);
			if (new_angle >= 0.0f) {
				object.transform.rotation.y = new_angle;
			}
		}
	} else if (walking_around && target_path.empty()) {
		walk_around(world, object);
	}
}

void character_object::start_path_movement(const std::vector<no::vector2i>& path) {
	target_path = path;
	tiles_moved = 0;
	walk_around_timer.start();
}

void character_object::walk_around(world_state& world, game_object& object) {
	no::random_number_generator random;
	if (walk_around_timer.has_started() && walk_around_timer.milliseconds() < random.next(4000, 5000)) {
		return;
	}
	no::vector2i distance{ random.next(-8, 8), random.next(-8, 8) };
	start_path_movement(world.path_between(object.tile(), walking_around_center + distance));
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

bool character_object::is_moving() const {
	return target_path.size() > 0;
}

bool character_object::in_combat() const {
	return last_combat_event_timer.seconds() < 5 && alive_timer.seconds() > 5;
}

bool character_object::is_fishing() const {
	return fishing;
}

character_stat& character_object::stat(stat_type stat) {
	return stats[(size_t)stat];
}
