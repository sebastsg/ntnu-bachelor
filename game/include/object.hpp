#pragma once

#include "transform.hpp"
#include "io.hpp"

class world_state;

class game_object {
public:

	no::transform transform;

	game_object(world_state& world);

	virtual ~game_object() = default;

	virtual void update() = 0;
	virtual void write(no::io_stream& stream) const;
	virtual void read(no::io_stream& stream);

	float x() const;
	float y() const;
	float z() const;
	int id() const;

	no::vector2i tile() const;

protected:

	world_state& world;
	int object_id = -1;

};

class decoration_object : public game_object {
public:

	std::string model;
	std::string texture;

	decoration_object(world_state& world);

	void update() override;
	void write(no::io_stream& stream) const override;
	void read(no::io_stream& stream) override;

};
