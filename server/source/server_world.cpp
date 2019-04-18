#include "server_world.hpp"
#include "pathfinding.hpp"
#include "server.hpp"

server_world::server_world(server_state& server, const std::string& name) : server{ server }, combat{ *this } {
	this->name = name;
	objects.load();
	terrain.load(1);
}

void server_world::update() {
	world_state::update();
	combat.update();
	update_fishing();
	objects.for_each([this](character_object* character) {
		auto& object = objects.object(character->object_id);
		update_combat_movement(*character, object);
		update_random_walk_movement(*character, object);
	});
}

void server_world::update_fishing() {
	for (auto& fisher : fishers) {
		if (fisher.last_progress.seconds() < 2 || fisher.finished) {
			continue;
		}
		auto& object = objects.object(fisher.player_instance_id);
		no::vector2i tile = object.tile();
		bool left = (tile.x < fisher.bait_tile.x);
		bool right = (tile.x > fisher.bait_tile.x);
		bool up = (tile.y < fisher.bait_tile.y);
		bool down = (tile.y > fisher.bait_tile.y);
		fisher.bait_tile.x += (left ? -1 : (right ? 1 : 0));
		fisher.bait_tile.y += (up ? -1 : (down ? 1 : 0));
		if (fisher.bait_tile == tile) {
			fisher.finished = true;
		}
		if (!terrain.tile_at(fisher.bait_tile).is_water()) {
			fisher.finished = true;
		}
		fisher.last_progress.start();
	}
}

void server_world::update_combat_movement(character_object& character, game_object& object) {
	if (!combat.is_in_combat(character.object_id)) {
		return;
	}
	int attacker_id = combat.find_attacker(character.object_id);
	int target_id = attacker_id != character.object_id ? attacker_id : combat.find_target(character.object_id);
	auto& target = objects.object(target_id);
	no::vector2i target_tile = target_tile_at_distance(target, object, 2);
	if (character.tiles_moved > 1 || character.target_path.empty()) {
		if (target_tile != target.tile()) {
			auto path = path_between(object.tile(), target_tile);
			character.start_path_movement(path);
			events.move.emplace_and_push(character.object_id, path);
		}
	}
	float new_angle = angle_to_goal(object.tile(), target_tile);
	if (object.transform.rotation.y != new_angle) {
		object.transform.rotation.y = new_angle;
		events.rotate.emplace_and_push(character.object_id, object.transform.rotation);
	}
}

void server_world::update_random_walk_movement(character_object& character, game_object& object) {
	if (!character.walking_around || !character.target_path.empty()) {
		return;
	}
	no::random_number_generator random;
	if (character.walk_around_timer.has_started() && character.walk_around_timer.milliseconds() < random.next(4000, 5000)) {
		return;
	}
	no::vector2i distance{ random.next(-8, 8), random.next(-8, 8) };
	character.start_path_movement(path_between(object.tile(), character.walking_around_center + distance));
}
