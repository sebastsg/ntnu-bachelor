#include "combat.hpp"
#include "server.hpp"
#include "character.hpp"

active_combat::active_combat(int attacker_id, int target_id, server_world& world) :
	attacker_id(attacker_id), target_id(target_id), world(&world) {
	last_hit.start();
}

bool active_combat::can_hit()const {
	return last_hit.milliseconds() > 1500;
}

bool active_combat::is_target_in_range() const {
	auto& attacker = world->objects.object(attacker_id);
	auto& target = world->objects.object(target_id);
	no::vector2i distance = attacker.tile() - target.tile();
	if (std::abs(distance.x) > 2 || std::abs(distance.y) > 2) {
		return false;
	}
	return true;
}

int active_combat::hit() {
	auto attacker = world->objects.character(attacker_id);
	auto target = world->objects.character(target_id);
	last_hit.start();
	int damage = std::rand() % 4;
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

combat_system::combat_system(server_world& world_) : world(world_) {
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
			}
			combat.next_turn();
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
