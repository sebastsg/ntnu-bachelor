#include "skeletal.hpp"

namespace no {

static glm::mat4 interpolate_positions(float factor, const vector3f& begin, const vector3f& end) {
	ASSERT(factor >= 0.0f && factor <= 1.0f);
	const vector3f delta = end - begin;
	const vector3f interpolated = factor * delta;
	const vector3f translation = begin + interpolated;
	return glm::translate(glm::mat4{ 1.0f }, { translation.x, translation.y, translation.z });
}

static glm::mat4 interpolate_rotations(float factor, const glm::quat& begin, const glm::quat& end) {
	ASSERT(factor >= 0.0f && factor <= 1.0f);
	return glm::mat4_cast(glm::normalize(glm::slerp(begin, end, factor)));
}

static glm::mat4 interpolate_scales(float factor, const vector3f& begin, const vector3f& end) {
	ASSERT(factor >= 0.0f && factor <= 1.0f);
	const vector3f delta = end - begin;
	const vector3f interpolated = factor * delta;
	const vector3f scale = begin + interpolated;
	return glm::scale(glm::mat4{ 1.0f }, { scale.x, scale.y, scale.z });
}

skeletal_animator::skeletal_animator(const model& skeleton) : skeleton(skeleton) {

}

int skeletal_animator::add() {
	for (int i = 0; i < (int)animations.size(); i++) {
		if (!animations[i].active) {
			animations[i] = {};
			active_count++;
			return i;
		}
	}
	animations.emplace_back();
	active_count++;
	return (int)animations.size() - 1;
}

void skeletal_animator::erase(int id) {
	if (id >= 0 && id < (int)animations.size()) {
		animations[id].active = false;
		active_count--;
	}
}

skeletal_animation& skeletal_animator::get(int id) {
	return animations[id];
}

int skeletal_animator::count() const {
	return active_count;
}

bool skeletal_animator::can_animate(int id) const {
	if (id >= (int)animations.size()) {
		return false;
	}
	if (animations[id].reference >= (int)skeleton.animations.size()) {
		return false;
	}
	if (animations[id].reference < 0) {
		return false;
	}
	return true;
}

void skeletal_animator::reset(int id) {
	animations[id].play_timer.start();
}

bool skeletal_animator::will_be_reset(int id) const {
	auto& animation = skeleton.animations[animations[id].reference];
	double seconds = (double)animations[id].play_timer.milliseconds() * 0.001;
	double play_duration = seconds * (double)animation.ticks_per_second;
	return play_duration >= (double)animation.duration;
}

void skeletal_animator::draw() const {
	skeleton.bind();
	for (auto& animation : animations) {
		if (!animation.active) {
			continue;
		}
		no::set_shader_model(animation.transform);
		shader.bones.set(animation.bones, animation.bone_count);
		skeleton.draw();
	}
}

void skeletal_animator::play(int id, int animation_index, int loops) {
	auto& animation = animations[id];
	if (animation.reference == animation_index) {
		return;
	}
	ASSERT(animation_index >= 0 && animation_index < skeleton.total_animations());
	animation.play_timer.start();
	animation.loops_completed = 0;
	animation.loops_assigned = loops;
	animation.reference = animation_index;
}

void skeletal_animator::play(int id, const std::string& animation_name, int loops) {
	play(id, skeleton.index_of_animation(animation_name), loops);
}

void skeletal_animator::animate() {
	for (auto& animation : animations) {
		animate(animation);
	}
}

glm::mat4 skeletal_animator::next_interpolated_position(skeletal_animation& animation, int node_index) {
	auto& node = skeleton.animations[animation.reference].channels[node_index];
	for (int p = 0; p < (int)node.positions.size() - 1; p++) {
		if (node.positions[p + 1].time > animation.time) {
			auto& current = node.positions[p];
			auto& next = node.positions[p + 1];
			float delta_time = next.time - current.time;
			float factor = (animation.time - current.time) / delta_time;
			return interpolate_positions(factor, current.position, next.position);
		}
	}
	if (node.positions.empty()) {
		return glm::mat4{ 1.0f };
	}
	auto& translation = node.positions.front().position;
	return glm::translate(glm::mat4{ 1.0f }, { translation.x, translation.y, translation.z });
}

glm::mat4 skeletal_animator::next_interpolated_rotation(skeletal_animation& animation, int node_index) {
	auto& node = skeleton.animations[animation.reference].channels[node_index];
	for (int r = 0; r < (int)node.rotations.size() - 1; r++) {
		if (node.rotations[r + 1].time > animation.time) {
			auto& current = node.rotations[r];
			auto& next = node.rotations[r + 1];
			float delta_time = next.time - current.time;
			float factor = (animation.time - current.time) / delta_time;
			return interpolate_rotations(factor, current.rotation, next.rotation);
		}
	}
	if (node.rotations.empty()) {
		return glm::mat4{ 1.0f };
	}
	return glm::mat4_cast(node.rotations.front().rotation);
}

glm::mat4 skeletal_animator::next_interpolated_scale(skeletal_animation& animation, int node_index) {
	auto& node = skeleton.animations[animation.reference].channels[node_index];
	for (int s = 0; s < (int)node.scales.size() - 1; s++) {
		if (node.scales[s + 1].time > animation.time) {
			auto& current = node.scales[s];
			auto& next = node.scales[s + 1];
			float delta_time = next.time - current.time;
			float factor = (animation.time - current.time) / delta_time;
			return interpolate_scales(factor, current.scale, next.scale);
		}
	}
	if (node.scales.empty()) {
		return glm::mat4{ 1.0f };
	}
	auto& scale = node.scales.front().scale;
	return glm::scale(glm::mat4{ 1.0f }, { scale.x, scale.y, scale.z });
}

void skeletal_animator::animate_node(skeletal_animation& animation, int parent, int node_index) {
	auto& reference = skeleton.animations[animation.reference];
	glm::mat4 node_transform = skeleton.nodes[node_index].transform;
	auto& node = reference.channels[node_index];
	if (node.positions.size() > 0 || node.rotations.size() > 0 || node.scales.size() > 0) {
		glm::mat4 translation = next_interpolated_position(animation, node_index);
		glm::mat4 rotation = next_interpolated_rotation(animation, node_index);
		glm::mat4 scale = next_interpolated_scale(animation, node_index);
		node_transform = translation * rotation * scale;
	}
	animation.transforms[node_index] = animation.transforms[parent] * node_transform;
	if (node.bone != -1) {
		if (animation.is_attachment) {
			animation.bones[node.bone] = animation.transforms[node_index] * animation.attachment.parent_bone * animation.attachment.child_bone * skeleton.root_transform;
		} else {
			animation.bones[node.bone] = skeleton.root_transform * animation.transforms[node_index] * skeleton.bones[node.bone];
		}
	}
	for (int child : skeleton.nodes[node_index].children) {
		animate_node(animation, node_index, child);
	}
}

void skeletal_animator::animate(skeletal_animation& animation) {
	if (!animation.active || animation.reference == -1) {
		return;
	}
	auto& reference = skeleton.animations[animation.reference];
	double seconds = (double)animation.play_timer.milliseconds() * 0.001;
	double play_duration = seconds * (double)reference.ticks_per_second;
	animation.time = (float)std::fmod(play_duration, (double)reference.duration);
	animation.transforms[0] = animation.root_transform;
	animation.bone_count = (int)skeleton.bones.size();
	animation.transform_count = skeleton.total_nodes();
	animate_node(animation, 0, 0);
}

void bone_attachment::update() {
	glm::mat4 translate{ glm::translate(glm::mat4{ 1.0f }, { position.x, position.y, position.z }) };
	glm::mat4 rotate{ glm::mat4_cast(glm::normalize(rotation)) };
	child_bone = translate * rotate;
}

bool bone_attachment_mapping::is_same_mapping(const bone_attachment_mapping& that) const {
	if (root_model != that.root_model) {
		return false;
	}
	if (attached_model != that.attached_model || attached_animation != that.attached_animation) {
		return false;
	}
	return true;
}

std::string bone_attachment_mapping::mapping_string() const {
	return root_model + "." + root_animation + " -> " + attached_model + "." + attached_animation;
}

void bone_attachment_mapping_list::save(const std::string& path) {
	io_stream stream;
	stream.write((int32_t)mappings.size());
	for (auto& attachment : mappings) {
		stream.write(attachment.root_model);
		stream.write(attachment.root_animation);
		stream.write(attachment.attached_model);
		stream.write(attachment.attached_animation);
		stream.write((int32_t)attachment.attached_to_channel);
		stream.write(attachment.position);
		stream.write(attachment.rotation);
	}
	file::write(path, stream);
}

void bone_attachment_mapping_list::load(const std::string& path) {
	io_stream stream;
	file::read(path, stream);
	if (stream.write_index() == 0) {
		return;
	}
	int32_t count = stream.read<int32_t>();
	for (int32_t i = 0; i < count; i++) {
		bone_attachment_mapping mapping;
		mapping.root_model = stream.read<std::string>();
		mapping.root_animation = stream.read<std::string>();
		mapping.attached_model = stream.read<std::string>();
		mapping.attached_animation = stream.read<std::string>();
		mapping.attached_to_channel = stream.read<int32_t>();
		mapping.position = stream.read<vector3f>();
		mapping.rotation = stream.read<glm::quat>();
		mappings.push_back(mapping);
	}
}

void bone_attachment_mapping_list::for_each(const std::function<bool(bone_attachment_mapping&)>& handler) {
	for (auto& mapping : mappings) {
		if (!handler(mapping)) {
			break;
		}
	}
}

void bone_attachment_mapping_list::remove_if(const std::function<bool(bone_attachment_mapping&)>& compare) {
	for (int i = 0; i < (int)mappings.size(); i++) {
		if (compare(mappings[i])) {
			mappings.erase(mappings.begin() + i);
			i--;
		}
	}
}

bool bone_attachment_mapping_list::exists(const bone_attachment_mapping& other) {
	for (auto& mapping : mappings) {
		if (mapping.is_same_mapping(other) && mapping.root_animation == other.root_animation) {
			return true;
		}
	}
	return false;
}

void bone_attachment_mapping_list::add(const bone_attachment_mapping& mapping) {
	if (!exists(mapping)) {
		mappings.push_back(mapping);
	}
}

bool bone_attachment_mapping_list::update(const model& root, int animation_index, const std::string& attachment_model, bone_attachment& attachment) const {
	auto& root_animation = root.animation(animation_index);
	for (auto& mapping : mappings) {
		if (mapping.root_model != root.name() || mapping.root_animation != root_animation.name) {
			continue;
		}
		if (mapping.attached_model != attachment_model) {
			continue;
		}
		attachment.parent = mapping.attached_to_channel;
		if (attachment.parent != -1) {
			attachment.parent_bone = root.bone(root_animation.channels[attachment.parent].bone);
		}
		attachment.position = mapping.position;
		attachment.rotation = mapping.rotation;
		attachment.update();
		return true;
	}
	return false;
}

std::string bone_attachment_mapping_list::find_root_animation(const std::string& root_model, const std::string& attached_model, const std::string& attached_animation) const {
	for (auto& mapping : mappings) {
		if (mapping.root_model != root_model) {
			continue;
		}
		if (mapping.attached_model != attached_model) {
			continue;
		}
		if (mapping.attached_animation != attached_animation) {
			continue;
		}
		return mapping.root_animation;
	}
	return "";
}

}
