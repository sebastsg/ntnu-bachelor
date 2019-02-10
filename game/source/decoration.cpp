#include "decoration.hpp"

void decoration_object::update() {

}

void decoration_object::write(no::io_stream& stream) const {
	game_object::write(stream);
}

void decoration_object::read(no::io_stream& stream) {
	game_object::read(stream);
}
