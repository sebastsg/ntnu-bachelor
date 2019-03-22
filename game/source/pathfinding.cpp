#include "pathfinding.hpp"
#include "world.hpp"

path_node::path_node(no::vector2i to, no::vector2i tile) : tile(tile) {
	h = tile.to<float>().distance_to(to.to<float>());
	f = h;
}

path_node::path_node(no::vector2i to, no::vector2i tile, path_node& parent, int parent_index) :
	tile(tile), parent((unsigned short)parent_index) {
	g = tile.to<float>().distance_to(parent.tile.to<float>()) + parent.g;
	h = tile.to<float>().distance_to(to.to<float>());
	f = g + h;
}

pathfinder::pathfinder(const world_terrain& terrain) : terrain(terrain) {
	
}

bool pathfinder::can_search(no::vector2i from, no::vector2i to) const {
	return max_search_area > std::abs(from.x - to.x) || max_search_area > std::abs(from.y - to.y);
}

std::vector<no::vector2i> pathfinder::find_path(no::vector2i from, no::vector2i to) {
	if (!can_search(from, to) || from == to) {
		return {};
	}
	no::vector2i original_to{ to };
	while (terrain.tiles().at(to.x, to.y).is_solid()) {
		to.x += (from.x > original_to.x ? 1 : -1);
		to.y += (from.y > original_to.y ? 1 : -1);
	}
	min_area = from - max_search_area;
	max_area = from + max_search_area;
	target_reached = false;
	nodes.clear();
	nodes.emplace_back(to, from).state = path_node::marked;
	previous = path_node::no_parent;
	current = 0;
	while (current != path_node::no_parent) {
		search_neighbours(to);
		if (target_reached) {
			break;
		}
		find_best_open();
	}
	return traverse_path(current == path_node::no_parent ? previous : current);
}

void pathfinder::search_neighbours(no::vector2i to) {
	terrain.tiles().for_each_neighbour(nodes[current].tile, [&](no::vector2i tile_index, const world_tile& tile) {
		if (tile.is_solid() || target_reached) {
			return;
		}
		if (tile_index.x < min_area.x || tile_index.y < min_area.y) {
			return;
		}
		if (tile_index.x > max_area.x || tile_index.y > max_area.y) {
			return;
		}
		if (tile_index == to) {
			nodes.emplace_back(to, to, nodes[current], current).state = path_node::marked;
			current = (int)nodes.size() - 1;
			target_reached = true;
			return;
		}
		path_node neighbour{ to, tile_index, nodes[current], current };
		for (auto& node : nodes) {
			if (node.state == path_node::opened && neighbour.tile == node.tile) {
				return; // already opened
			}
			if (node.state == path_node::closed && neighbour.tile == node.tile && neighbour.f > node.f) {
				return; // more expensive
			}
		}
		nodes.emplace_back(neighbour).state = path_node::opened;
	});
}

void pathfinder::find_best_open() {
	float min_f = FLT_MAX;
	nodes.emplace_back(nodes[current]).state = path_node::closed;
	previous = current;
	current = path_node::no_parent;
	for (int i = 0; i < (int)nodes.size(); i++) {
		if (nodes[i].state == path_node::opened && min_f > nodes[i].f) {
			current = i;
			min_f = nodes[i].f;
		}
	}
	if (current != path_node::no_parent) {
		nodes[current].state = path_node::marked;
	}
}

std::vector<no::vector2i> pathfinder::traverse_path(int index) const {
	std::vector<no::vector2i> result;
	while (index != path_node::no_parent) {
		result.push_back(nodes[index].tile);
		index = nodes[index].parent;
	}
	return result;
}

float angle_to_goal(no::vector2i from, no::vector2i to) {
	bool left = from.x > to.x;
	bool right = to.x > from.x;
	if (right == left) {
		right = false;
		left = false;
	}
	bool up = from.y > to.y;
	bool down = to.y > from.y;
	if (up == down) {
		up = false;
		down = false;
	}
	if (left && up) {
		return directions::north_west;
	} else if (right && up) {
		return directions::north_east;
	} else if (left && down) {
		return directions::south_west;
	} else if (right && down) {
		return directions::south_east;
	} else if (left) {
		return directions::west;
	} else if (right) {
		return directions::east;
	} else if (down) {
		return directions::south;
	} else if (up) {
		return directions::north;
	}
	return -1.0f;
}

no::vector2f distance_to_goal(const no::vector3f& current, const no::vector3f& goal) {
	const float speed = 0.05f;
	float x = goal.x - current.x;
	float z = goal.z - current.z;
	no::vector2f distance;
	if (std::abs(x) >= speed) {
		distance.x = (x > 0.0f ? speed : -speed);
	}
	if (std::abs(z) >= speed) {
		distance.y = (z > 0.0f ? speed : -speed);
	}
	return distance;
}

bool move_towards_target(no::transform3& transform, std::vector<no::vector2i>& path) {
	if (path.empty()) {
		return false;
	}
	float speed = 0.05f;
	no::vector2i current_target = path.back();
	no::vector3f target_position = tile_index_to_world_position(current_target);
	no::vector2f distance = distance_to_goal(transform.position, target_position);
	bool moving = (std::abs(distance.x) > 0.0f || std::abs(distance.y) > 0.0f);
	if (!moving) {
		path.pop_back();
		return path.empty() ? false : move_towards_target(transform, path);
	}
	transform.position.x += distance.x;
	transform.position.z += distance.y;
	float new_angle = angle_to_goal(world_position_to_tile_index(transform.position), current_target);
	if (new_angle >= 0.0f) {
		transform.rotation.y = new_angle;
	}
	return true;
}

no::vector2i target_tile_at_distance(const game_object& target, const game_object& follower, int distance) {
	no::vector2i target_tile = target.tile();
	no::vector2i delta = follower.tile() - target_tile;
	for (int i = 0; i < distance; i++) {
		if (delta.x > 1) {
			target_tile.x++; // to west of target
			delta.x--;
		} else if (delta.x < -1) {
			target_tile.x--; // to east of target
			delta.x++;
		}
		if (delta.y > 1) {
			target_tile.y++; // to south of target
			delta.y--;
		} else if (delta.y < -1) {
			target_tile.y--; // to north of target
			delta.y++;
		}
	}
	return target_tile;
}
