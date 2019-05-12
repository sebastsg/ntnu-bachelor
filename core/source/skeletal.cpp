#include "skeletal.hpp"

#if ENABLE_GRAPHICS

namespace no {

static FORCE_INLINE glm::mat4 interpolate_positions(float factor, const vector3f& begin, const vector3f& end) {
	const vector3f delta = end - begin;
	const vector3f interpolated = factor * delta;
	const vector3f translation = begin + interpolated;
	return glm::translate(glm::mat4{ 1.0f }, { translation.x, translation.y, translation.z });
}

static FORCE_INLINE glm::mat4 interpolate_rotations(float factor, const glm::quat& begin, const glm::quat& end) {
	return glm::mat4_cast(glm::normalize(glm::slerp(begin, end, factor)));
}

static FORCE_INLINE glm::mat4 interpolate_scales(float factor, const vector3f& begin, const vector3f& end) {
	const vector3f delta = end - begin;
	const vector3f interpolated = factor * delta;
	const vector3f scale = begin + interpolated;
	return glm::scale(glm::mat4{ 1.0f }, { scale.x, scale.y, scale.z });
}

skeletal_animation::skeletal_animation(int id) : id{ id } {

}

void skeletal_animation::apply_update(skeletal_animation_update& update) {
	is_attachment = update.is_attachment;
	root_transform = update.root_transform;
	attachment = update.attachment;
	reference = update.reference;
	if (update.reset) {
		play_timer.start();
		loops_completed = 0;
		loops_assigned = update.loops_if_reset;
		update.reset = false;
	}
}

synced_skeletal_animation::synced_skeletal_animation(const skeletal_animation& animation) {
	active = animation.active;
	id = animation.id;
	reference = animation.reference;
	attachment = animation.attachment;
	std::copy(std::begin(animation.bones), std::end(animation.bones), std::begin(bones));
	std::copy(std::begin(animation.transforms), std::end(animation.transforms), std::begin(transforms));
	root_transform = animation.root_transform;
	bone_count = animation.bone_count;
	transform_count = animation.transform_count;
	played_for = animation.play_timer;
}

skeletal_animator::skeletal_animator(const model& skeleton) : skeleton{ skeleton } {

}

void skeletal_animator::animate() {
	std::lock_guard lock{ animating };
	for (auto& animation : animations) {
		animate(animation);
	}
}

void skeletal_animator::sync() {
	std::lock_guard lock{ animating };
	std::lock_guard sync{ reading_synced };
	for (auto& animation : animations) {
		animation.apply_update(animation_updates[animation.id]);
		synced_animations[animation.id] = animation;
	}
}

skeletal_animator::~skeletal_animator() {

}

int skeletal_animator::add() {
	std::lock_guard lock{ animating };
	for (int i = 0; i < (int)animations.size(); i++) {
		if (!animations[i].active) {
			animations[i] = { i };
			synced_animations[i] = animations[i];
			active_count++;
			return i;
		}
	}
	int id = (int)animations.size();
	model_transforms.emplace_back();
	animations.emplace_back(id);
	synced_animations.emplace_back(id);
	animation_updates.emplace_back();
	active_count++;
	return id;
}

void skeletal_animator::erase(int id) {
	std::lock_guard lock{ animating };
	if (id >= 0 && id < (int)animations.size()) {
		animations[id].active = false;
		synced_animations[id] = animations[id];
		active_count--;
	}
}

void skeletal_animator::set_transform(int id, const no::transform3& transform) {
	model_transforms[id] = transform;
}

void skeletal_animator::set_is_attachment(int id, bool attachment) {
	animation_updates[id].is_attachment = attachment;
}

void skeletal_animator::set_root_transform(int id, const glm::mat4& transform) {
	animation_updates[id].root_transform = transform;
}

glm::mat4 skeletal_animator::get_transform(int id, int transform) const {
	return synced_animations[id].transforms[transform];
}

void skeletal_animator::set_attachment_bone(int id, const bone_attachment& attachment) {
	animation_updates[id].attachment = attachment;
}

bone_attachment skeletal_animator::get_attachment_bone(int id) const {
	return synced_animations[id].attachment;
}

int skeletal_animator::count() const {
	return active_count;
}

bool skeletal_animator::can_animate(int id) const {
	std::lock_guard lock{ reading_synced };
	if (id < 0 || id >= (int)synced_animations.size()) {
		return false;
	}
	if (synced_animations[id].reference >= (int)skeleton.animations.size()) {
		return false;
	}
	if (synced_animations[id].reference < 0) {
		return false;
	}
	return true;
}

void skeletal_animator::reset(int id) {
	std::lock_guard lock{ reading_synced };
	animation_updates[id].reset = true;
}

bool skeletal_animator::will_be_reset(int id) const {
	std::lock_guard lock{ reading_synced };
	if (synced_animations[id].reference < 0) {
		return false;
	}
	auto& animation = skeleton.animations[synced_animations[id].reference];
	double seconds = (double)synced_animations[id].played_for.milliseconds() * 0.001;
	double play_duration = seconds * (double)animation.ticks_per_second;
	return play_duration >= (double)animation.duration;
}

void skeletal_animator::draw() const {
	std::lock_guard lock{ reading_synced };
	skeleton.bind();
	for (auto& animation : synced_animations) {
		if (!animation.active) {
			continue;
		}
		no::set_shader_model(model_transforms[animation.id]);
		shader.bones.set(animation.bones, animation.bone_count);
		skeleton.draw();
	}
}

void skeletal_animator::play(int id, int animation_index, int loops) {
	std::lock_guard lock{ reading_synced };
	auto& animation = animation_updates[id];
	if (animation.reference != animation_index) {
		animation.reset = true;
		animation.loops_if_reset = loops;
		animation.reference = animation_index;
	}
}

void skeletal_animator::play(int id, const std::string& animation_name, int loops) {
	play(id, skeleton.index_of_animation(animation_name), loops);
}

glm::mat4 skeletal_animator::next_interpolated_position(skeletal_animation& animation, int node_index) {
	auto& node = skeleton.animations[animation.reference].channels[node_index];
	const int count = (int)node.positions.size();
	for (int p = animation.next_p; p < count; p++) {
		if (node.positions[p].time > animation.time) {
			animation.next_p = p;
			auto& current = node.positions[p - 1];
			auto& next = node.positions[p];
			float delta_time = next.time - current.time;
			float factor = (animation.time - current.time) / delta_time;
			return interpolate_positions(factor, current.position, next.position);
		}
	}
	if (node.positions.empty()) {
		return glm::mat4{ 1.0f };
	}
	animation.next_p = 1;
	auto& translation = node.positions.front().position;
	return glm::translate(glm::mat4{ 1.0f }, { translation.x, translation.y, translation.z });
}

glm::mat4 skeletal_animator::next_interpolated_rotation(skeletal_animation& animation, int node_index) {
	auto& node = skeleton.animations[animation.reference].channels[node_index];
	const int count = (int)node.rotations.size();
	for (int r = animation.next_r; r < count; r++) {
		if (node.rotations[r].time > animation.time) {
			animation.next_r = r;
			auto& current = node.rotations[r - 1];
			auto& next = node.rotations[r];
			float delta_time = next.time - current.time;
			float factor = (animation.time - current.time) / delta_time;
			return interpolate_rotations(factor, current.rotation, next.rotation);
		}
	}
	if (node.rotations.empty()) {
		return glm::mat4{ 1.0f };
	}
	animation.next_r = 1;
	return glm::mat4_cast(node.rotations.front().rotation);
}

glm::mat4 skeletal_animator::next_interpolated_scale(skeletal_animation& animation, int node_index) {
	auto& node = skeleton.animations[animation.reference].channels[node_index];
	const int count = (int)node.scales.size();
	for (int s = animation.next_s; s < count; s++) {
		if (node.scales[s].time > animation.time) {
			animation.next_s = s;
			auto& current = node.scales[s - 1];
			auto& next = node.scales[s];
			float delta_time = next.time - current.time;
			float factor = (animation.time - current.time) / delta_time;
			return interpolate_scales(factor, current.scale, next.scale);
		}
	}
	if (node.scales.empty()) {
		return glm::mat4{ 1.0f };
	}
	animation.next_s = 1;
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
	float time = animation.time;
	animation.time = (float)std::fmod(play_duration, (double)reference.duration);
	if (time > animation.time) {
		animation.next_p = 1;
		animation.next_r = 1;
		animation.next_s = 1;
	}
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
	if (animation_index < 0) {
		return false;
	}
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

#endif
