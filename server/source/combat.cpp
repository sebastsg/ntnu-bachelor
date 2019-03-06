#include "combat.hpp"
#include "world.hpp"
#include "character.hpp"

active_combat::active_combat(int attacker_id, int target_id, world_state& world) :
	attacker_id(attacker_id), target_id(target_id), world(&world) {
	last_hit.start();
}

bool active_combat::can_hit()const {
	return last_hit.milliseconds() > 1500;
}

int active_combat::hit() {
	auto attacker = (character_object*)world->objects.find(attacker_id);
	auto target = (character_object*)world->objects.find(target_id);
	last_hit.start();
	int damage = std::rand() % 4;
	target->health.add(-damage);
	std::swap(attacker_id, target_id);
	return damage;
}

combat_system::combat_system(world_state& world_) : world(world_) {
	object_remove_event = world.objects.events.remove.listen([this](const world_objects::remove_event& event) {
		stop_all(event.object->id());
	});
}

combat_system::~combat_system() {
	world.objects.events.remove.ignore(object_remove_event);
}

void combat_system::update() {
	for (auto& combat : combats) {
		if (combat.can_hit()) {
			events.hit.emit(combat.attacker_id, combat.target_id, combat.hit());
		}
	}
}

void combat_system::add(int attacker_id, int target_id) {
	ASSERT(attacker_id != -1);
	ASSERT(target_id != -1);
	for (auto& combat : combats) {
		if (combat.attacker_id == attacker_id) {
			return;
		}
		if (combat.target_id == target_id) {
			return; // todo: allow multi combat in future?
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
