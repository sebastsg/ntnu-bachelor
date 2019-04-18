#include "combat.hpp"
#include "server.hpp"
#include "character.hpp"

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
	return damage;
}

void active_combat::next_turn() {
	std::swap(attacker_id, target_id);
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

int combat_system::find_target(int object_id) const {
	for (auto& combat : combats) {
		if (combat.target_id == object_id) {
			return object_id;
		}
		if (combat.attacker_id == object_id) {
			return combat.target_id;
		}
	}
	return -1;
}

int combat_system::find_attacker(int object_id) const {
	for (auto& combat : combats) {
		if (combat.target_id == object_id) {
			return combat.attacker_id;
		}
		if (combat.attacker_id == object_id) {
			return object_id;
		}
	}
	return -1;
}
