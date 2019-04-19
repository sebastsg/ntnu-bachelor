#include "combat.hpp"
#include "server.hpp"
#include "character.hpp"
#include "pathfinding.hpp"

active_combat::active_combat(int attacker_id, int target_id, server_world& world) :
	attacker_id(attacker_id), target_id(target_id), world(&world) {
	last_hit.start();
}

bool active_combat::can_hit()const {
	return last_hit.milliseconds() > 2000;
}

bool active_combat::is_target_in_range() const {
	auto& attacker = world->objects.object(attacker_id);
	auto& target = world->objects.object(target_id);
	no::vector2i distance = attacker.tile() - target.tile();
	if (std::abs(distance.x) > 3 || std::abs(distance.y) > 3) {
		return false;
	}
	return true;
}

int active_combat::hit() {
	auto attacker = world->objects.character(attacker_id);
	auto target = world->objects.character(target_id);
	last_hit.start();
	int accuracy = 1 + attacker->equipment.accuracy();
	int power = 1 + attacker->equipment.power();
	int protection = 1 + target->equipment.protection();
	protection += target->stat(stat_type::defensive).effective();
	item_instance weapon = attacker->equipment.get(equipment_slot::right_hand);
	if (weapon.definition_id != -1) {
		switch (weapon.definition().equipment) {
		case equipment_type::axe:
			accuracy += attacker->stat(stat_type::axe).effective();
			power += attacker->stat(stat_type::axe).effective();
			break;
		case equipment_type::sword:
			accuracy += attacker->stat(stat_type::sword).effective();
			power += attacker->stat(stat_type::sword).effective();
			break;
		case equipment_type::spear:
			accuracy += attacker->stat(stat_type::spear).effective();
			power += attacker->stat(stat_type::spear).effective();
			break;
		}
	}
	float hit_accuracy = 0.5f + random.next<float>((float)accuracy) / 100.0f;
	float hit_power = random.next<float>((float)power);
	float hit_protection = random.next<float>((float)protection);
	float hit_damage = (hit_accuracy * hit_power) - hit_protection;
	int damage = std::max(0, (int)hit_damage);
	target->stat(stat_type::health).add_effective(-damage);
	attacker->add_combat_experience(damage);
	if (target->stat(stat_type::health).effective() < 1) {
		world->events.kill.move_and_push({ attacker_id, target_id });
	}
	target->target_path.clear();
	return damage;
}

void active_combat::next_turn() {
	std::swap(attacker_id, target_id);
}

void active_combat::update_movement() {
	auto& attacker_object = world->objects.object(attacker_id);
	auto& target_object = world->objects.object(target_id);
	auto attacker = world->objects.character(attacker_id);
	auto target = world->objects.character(target_id);

	no::vector2i target_tile = target_object.tile();

	float attacker_angle = angle_to_goal(attacker_object.tile(), target_tile);
	if (attacker_angle >= 0.0f && attacker_object.transform.rotation.y != attacker_angle) {
		attacker_object.transform.rotation.y = attacker_angle;
		world->events.rotate.emplace_and_push(attacker->object_id, attacker_object.transform.rotation);
	}

	float target_angle = angle_to_goal(target_tile, attacker_object.tile());
	if (target_angle >= 0.0f && target_object.transform.rotation.y != target_angle) {
		target_object.transform.rotation.y = target_angle;
		world->events.rotate.emplace_and_push(target->object_id, target_object.transform.rotation);
	}

	if (!attacker->target_path.empty()) {
		return;
	}

	std::vector<no::vector2i> path;

	no::vector2i delta = target_tile - attacker_object.tile();
	bool too_close = (std::abs(delta.x) < 2 && std::abs(delta.y) < 2);
	if (too_close) {
		if (delta == 0) {
			int off_x = random.chance(0.5f) ? 2 : 0;
			int off_y = random.chance(0.5f) ? 2 : 0;
			if (off_x == 0) {
				off_y = random.chance(0.5f) ? 2 : -2;
			}
			if (off_y == 0) {
				off_x = random.chance(0.5f) ? 2 : -2;
			}
			if (off_x != 0 && off_y != 0) {
				off_y = 0;
			}
			path.emplace_back(target_tile.x + off_x, target_tile.y + off_y);
		} else {
			path.emplace_back(target_tile - delta);
		}
	} else {
		path = world->path_between(attacker_object.tile(), target_tile);
		for (int i = 0; i < 2 && !path.empty(); i++) {
			path.erase(path.begin());
		}
	}
	attacker->target_path = path;
	world->events.move.emplace_and_push(attacker_id, path);

	/*no::vector2i target_tile = target_tile_at_distance(target, object, 2);
	if (character.tiles_moved > 1 || character.target_path.empty()) {
		if (target_tile != target.tile()) {
			auto path = path_between(object.tile(), target_tile);
			character.start_path_movement(path);
			events.move.emplace_and_push(character.object_id, path);
		}
	}


	float new_angle = angle_to_goal(object.tile(), target_tile);
	if (new_angle >= 0.0f && object.transform.rotation.y != new_angle) {
		object.transform.rotation.y = new_angle;
		events.rotate.emplace_and_push(character.object_id, object.transform.rotation);
	}*/
}

combat_system::combat_system(server_world& world) : world{ world } {
	object_remove_event = world.objects.events.remove.listen([this](const game_object& object) {
		stop_all(object.instance_id);
	});
}

combat_system::~combat_system() {
	world.objects.events.remove.ignore(object_remove_event);
}

void combat_system::update() {
	for (auto& combat : combats) {
		combat.update_movement();
		if (combat.can_hit()) {
			auto target = world.objects.character(combat.target_id);
			if (target->stat(stat_type::health).effective() < 1) {
				continue;
			}
			if (combat.is_target_in_range()) {
				events.hit.emit(combat.attacker_id, combat.target_id, combat.hit());
				combat.next_turn();
			}
		}
	}
}

void combat_system::add(int attacker_id, int target_id) {
	ASSERT(attacker_id != -1);
	ASSERT(target_id != -1);
	for (auto& combat : combats) {
		if (combat.attacker_id == attacker_id || combat.target_id == target_id) {
			return;
		}
	}
	combats.emplace_back(attacker_id, target_id, world);
}

void combat_system::stop_all(int object_id) {
	for (int i = 0; i < (int)combats.size(); i++) {
		if (combats[i].attacker_id == object_id || combats[i].target_id == object_id) {
			combats.erase(combats.begin() + i);
			i--;
		}
	}
}

bool combat_system::is_in_combat(int object_id) const {
	for (auto& combat : combats) {
		if (combat.attacker_id == object_id || combat.target_id == object_id) {
			return true;
		}
	}
	return false;
}
