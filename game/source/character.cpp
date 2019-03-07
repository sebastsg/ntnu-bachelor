#include "character.hpp"
#include "world.hpp"
#include "pathfinding.hpp"

#include "assets.hpp"
#include "surface.hpp"

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

void character_object::update() {
	move_towards_target();
	auto target = world->objects.find(follow_object_id);
	if (target && (tiles_moved > 1 || target_path.empty())) {
		no::vector2i target_tile = target->tile();
		no::vector2i delta = tile() - target_tile;
		if (delta.x > 1) {
			target_tile.x++; // to west of target
		} else if (delta.x < -1) {
			target_tile.x--; // to east of target
		}
		if (delta.y > 1) {
			target_tile.y++; // to south of target
		} else if (delta.y < -1) {
			target_tile.y--; // to north of target
		}
		no::vector2i new_delta = tile() - target_tile;
		if (std::abs(new_delta.x) > follow_distance || std::abs(new_delta.y > follow_distance)) {
			pathfinder pathfind{ world->terrain };
			start_path_movement(pathfind.find_path(tile(), target_tile));
		} else {
			no::vector2i a = tile();
			no::vector2i b = target->tile();
			if (a.x > b.x && a.y > b.y) {
				transform.rotation.y = directions::north_west;
			} else if (a.x > b.x && a.y < b.y) {
				transform.rotation.y = directions::south_west;
			} else if (a.x < b.x && a.y > b.y) {
				transform.rotation.y = directions::north_east;
			} else if (a.x < b.x && a.y < b.y) {
				transform.rotation.y = directions::south_east;
			} else if (a.x > b.x) {
				transform.rotation.y = directions::west;
			} else if (a.x < b.x) {
				transform.rotation.y = directions::east;
			} else if (a.y > b.y) {
				transform.rotation.y = directions::north;
			} else if (a.y < b.y) {
				transform.rotation.y = directions::south;
			}
		}
	}
}

void character_object::write(no::io_stream& stream) const {
	game_object::write(stream);
	inventory.write(stream);
	equipment.write(stream);
	stream.write((int32_t)stat_type::total);
	for (auto& stat : stats) {
		stat.write(stream);
	}
}

void character_object::read(no::io_stream& stream) {
	game_object::read(stream);
	inventory.read(stream);
	equipment.read(stream);
	int count = stream.read<int32_t>();
	for (int i = 0; i < count; i++) {
		stats[i].read(stream);
	}
}

void character_object::equip_from_inventory(no::vector2i slot) {
	item_instance item = inventory.at(slot);
	item.stack = 0;
	inventory.remove_to(inventory.at(slot).stack, item);
	events.equip.emit(item);
	equipment.add_from(item);
}

void character_object::unequip_to_inventory(no::vector2i slot) {
	item_instance item = equipment.at(slot);
	item.stack = 0;
	equipment.remove_to(equipment.at(slot).stack, item);
	inventory.add_from(item);
	events.unequip.emit(item_definitions().get(item.definition_id).slot);
}

void character_object::equip(item_instance item) {
	equipment.add_from(item);
	events.equip.emit(item);
}

bool character_object::is_moving() const {
	return moving;
}

void character_object::start_path_movement(const std::vector<no::vector2i>& path) {
	target_path = path;
	tiles_moved = 0;
}

character_stat& character_object::stat(stat_type stat) {
	return stats[(size_t)stat];
}

void character_object::move_towards_target() {
	while (!target_path.empty() && target_path.back() < 0) {
		target_path.pop_back();
	}
	if (target_path.empty()) {
		return;
	}
	float speed = 0.05f;
	no::vector2i current_target = target_path.back();
	no::vector3f target_position = world->tile_index_to_world_position(current_target.x, current_target.y);
	float x_difference = target_position.x - transform.position.x + 0.5f;
	float z_difference = target_position.z - transform.position.z + 0.5f;
	moving = false;
	if (std::abs(x_difference) >= speed) {
		transform.position.x += (x_difference > 0.0f ? speed : -speed);
		moving = true;
	}
	if (std::abs(z_difference) >= speed) {
		transform.position.z += (z_difference > 0.0f ? speed : -speed);
		moving = true;
	}
	if (!moving) {
		target_path.pop_back();
		tiles_moved++;
		if (!target_path.empty()) {
			move_towards_target();
		}
		return;
	}
	no::vector2i from = tile();
	no::vector2i to = current_target;
	bool left = false;
	bool up = false;
	bool right = false;
	bool down = false;
	right = to.x > from.x;
	left = from.x > to.x;
	down = to.y > from.y;
	up = from.y > to.y;
	if (right == left) {
		right = false;
		left = false;
	}
	if (up == down) {
		up = false;
		down = false;
	}
	if (left && up) {
		transform.rotation.y = directions::north_west;
	} else if (right && up) {
		transform.rotation.y = directions::north_east;
	} else if (left && down) {
		transform.rotation.y = directions::south_west;
	} else if (right && down) {
		transform.rotation.y = directions::south_east;
	} else if (left) {
		transform.rotation.y = directions::west;
	} else if (right) {
		transform.rotation.y = directions::east;
	} else if (down) {
		transform.rotation.y = directions::south;
	} else if (up) {
		transform.rotation.y = directions::north;
	}
}
