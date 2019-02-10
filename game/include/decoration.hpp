#pragma once

#include "object.hpp"

class decoration_object : public game_object {
public:

	void update() override;
	void write(no::io_stream& stream) const override;
	void read(no::io_stream& stream) override;

};
