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
	if (!can_search(from, to) || from == to || terrain.is_out_of_bounds(to)) {
		return {};
	}
	no::vector2i original_to{ to };
	while (terrain.tile_at(to).is_solid()) {
		to.x += (from.x > to.x ? 1 : -1);
		to.y += (from.y > to.y ? 1 : -1);
		if (terrain.is_out_of_bounds(to)) {
			return {};
		}
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
	terrain.for_each_neighbour(nodes[current].tile, [&](no::vector2i tile_index, const world_tile& tile) {
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

float angle_to_goal(no::vector3f from, no::vector3f to) {
	const float z = from.z - to.z;
	const float x = from.x - to.x;
	float result = no::rad_to_deg(std::atan2(z, x));
	if (result > 180.0f) {
		result = 540.0f - result;
	} else {
		result = 180.0f - result;
	}
	return std::floor(result + 90.0f);
}

no::vector2f distance_to_goal(const no::vector3f& current, const no::vector3f& goal, float speed) {
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

bool move_towards_target(no::transform3& transform, std::vector<no::vector2i>& path, float speed) {
	if (path.empty()) {
		return false;
	}
	no::vector2i current_target = path.back();
	no::vector3f target_position = tile_index_to_world_position(current_target);
	no::vector2f distance = distance_to_goal(transform.position, target_position, speed);
	bool moving = (std::abs(distance.x) > 0.0f || std::abs(distance.y) > 0.0f);
	if (!moving) {
		path.pop_back();
		return path.empty() ? false : move_towards_target(transform, path, speed);
	}
	transform.position.x += distance.x;
	transform.position.z += distance.y;
	if (std::abs(transform.position.x - target_position.x) > 0.4f || std::abs(transform.position.z - target_position.z) > 0.4f) {
		transform.rotation.y = angle_to_goal(transform.position, target_position);
	}
	return true;
}
