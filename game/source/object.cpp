#include "object.hpp"
#include "world.hpp"

game_object::game_object(world_state& world) : world(world) {
	object_id = world.next_object_id();
}

void game_object::write(no::io_stream& stream) const {
	stream.write(transform);
	stream.write((int32_t)object_id);
}

void game_object::read(no::io_stream& stream) {
	transform = stream.read<no::transform>();
	object_id = stream.read<int32_t>();
}

float game_object::x() const {
	return transform.position.x;
}

float game_object::y() const {
	return transform.position.y;
}

float game_object::z() const {
	return transform.position.z;
}

int game_object::id() const {
	return object_id;
}

no::vector2i game_object::tile() const {
	return world.world_position_to_tile_index(x(), z());
}

decoration_object::decoration_object(world_state& world) : game_object(world) {
	transform.scale = 0.5f;
}

void decoration_object::update() {

}

void decoration_object::write(no::io_stream& stream) const {
	game_object::write(stream);
	stream.write(model);
	stream.write(texture);
}

void decoration_object::read(no::io_stream& stream) {
	game_object::read(stream);
	model = stream.read<std::string>();
	texture = stream.read<std::string>();
}
