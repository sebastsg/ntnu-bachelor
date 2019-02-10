#include "character.hpp"
#include "world.hpp"

#include "assets.hpp"
#include "surface.hpp"

void character_object::update() {
	move_towards_target();
}

void character_object::write(no::io_stream& stream) const {
	game_object::write(stream);
	inventory.write(stream);
	equipment.write(stream);
}

void character_object::read(no::io_stream& stream) {
	game_object::read(stream);
	inventory.read(stream);
	equipment.read(stream);
}

bool character_object::is_moving() const {
	return moving;
}

void character_object::start_movement_to(int x, int z) {
	target_x = x;
	target_z = z;
}

void character_object::move_towards_target() {
	if (target_x < 0 || target_z < 0) {
		return;
	}
	float speed = 0.05f;
	auto target_position = world->tile_index_to_world_position(target_x, target_z);
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
