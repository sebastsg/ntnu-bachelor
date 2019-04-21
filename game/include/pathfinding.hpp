#pragma once

#include "transform.hpp"

#include <vector>

class world_terrain;
struct game_object;

struct path_node {

	static const unsigned short unused = 0;
	static const unsigned short opened = 1;
	static const unsigned short closed = 2;
	static const unsigned short marked = 3;

	static const unsigned short no_parent = 0xFFFF;

	no::vector2i tile;
	unsigned short parent = no_parent;
	unsigned short state = unused;
	float g = 0.0f;
	float h = 0.0f;
	float f = 0.0f;

	path_node(no::vector2i to, no::vector2i tile);
	path_node(no::vector2i to, no::vector2i tile, path_node& parent, int parent_index);

};

class pathfinder {
public:

	static const int max_search_area = 50;

	pathfinder(const world_terrain& terrain);

	bool can_search(no::vector2i from, no::vector2i to) const;
	std::vector<no::vector2i> find_path(no::vector2i from, no::vector2i to);

private:

	void search_neighbours(no::vector2i to);
	void find_best_open();
	std::vector<no::vector2i> traverse_path(int index) const;

	const world_terrain& terrain;
	std::vector<path_node> nodes;
	bool target_reached = false;
	int current = path_node::no_parent;
	int previous = path_node::no_parent;
	no::vector2i min_area;
	no::vector2i max_area;

};

float angle_to_goal(no::vector3f from, no::vector3f to);
no::vector2f distance_to_goal(const no::vector3f& current, const no::vector3f& goal, float speed);
bool move_towards_target(no::transform3& transform, std::vector<no::vector2i>& path, float speed);
