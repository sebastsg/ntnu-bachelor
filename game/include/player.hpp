#pragma once

#include "transform.hpp"
#include "draw.hpp"

class world_state;

class game_object {
public:

	no::transform transform;

	game_object(world_state& world);

	virtual ~game_object() = default;

	virtual void update() = 0;

	float x() const {
		return transform.position.x;
	}

	float y() const {
		return transform.position.y;
	}

	float z() const {
		return transform.position.z;
	}

	no::vector2i tile() const;

protected:

	world_state& world;

};

class player_object : public game_object {
public:

	int player_id = 0;

	player_object(world_state& world);
	player_object(player_object&&) = default;
	~player_object() override;

	void update() override;

	bool is_moving() const;

	void start_movement_to(int x, int z);

private:

	void move_towards_target();

	int target_x = -1;
	int target_z = -1;

	bool moving = false;

};

class player_renderer {
public:

	player_renderer(const world_state& world);

	void draw();

private:

	no::model model;
	no::model sword_model;

	int idle = 0;
	int run = 0;

	const world_state& world;
	std::vector<no::model_instance> animations;

};
