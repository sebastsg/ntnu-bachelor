#include "player.hpp"
#include "world.hpp"

#include "assets.hpp"
#include "surface.hpp"

game_object::game_object(world_state& world) : world(world) {

}

no::vector2i game_object::tile() const {
	return world.world_position_to_tile_index(x(), z());
}

player_object::player_object(world_state& world) : game_object(world) {
	transform.scale = 0.5f;
}

player_object::~player_object() {

}

bool player_object::is_moving() const {
	return moving;
}

void player_object::move_towards_target() {
	if (target_x < 0 || target_z < 0) {
		return;
	}
	float speed = 0.05f;
	auto target_position = world.tile_index_to_world_position(target_x, target_z);
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
		target_x = -1;
		target_z = -1;
		return;
	}
	no::vector2i from = tile();
	no::vector2i to = { target_x, target_z };
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
		transform.rotation.y = 225.0f;
	} else if (right && up) {
		transform.rotation.y = 135.0f;
	} else if (left && down) {
		transform.rotation.y = 315.0f;
	} else if (right && down) {
		transform.rotation.y = 45.0f;
	} else if (left) {
		transform.rotation.y = 270.0f;
	} else if (right) {
		transform.rotation.y = 90.0f;
	} else if (down) {
		transform.rotation.y = 0.0f;
	} else if (up) {
		transform.rotation.y = 180.0f;
	}
}

void player_object::update() {
	move_towards_target();
}

void player_object::start_movement_to(int x, int z) {
	target_x = x;
	target_z = z;
}

player_renderer::player_renderer(const world_state& world) : world(world) {
	model.load(no::asset_path("models/player.nom"));
	sword_model.load(no::asset_path("models/sword.nom"));
	idle = model.index_of_animation("idle");
	run = model.index_of_animation("run");
}

void player_renderer::draw() {
	while (world.players.size() > animations.size()) {
		animations.emplace_back(model);
		animations.back().attach(15, sword_model, { 0.0761f, 0.5661f, 0.1151f }, { 0.595f, -0.476f, -0.464f, -0.452f });
	}
	int i = 0;
	for (auto& player : world.players) {
		if (player.second.is_moving()) {
			animations[i].start_animation(run);
		} else {
			animations[i].start_animation(idle);
		}
		animations[i].animate();
		no::set_shader_model(world.players.find(i)->second.transform);
		animations[i].draw();
		i++;
	}
}
