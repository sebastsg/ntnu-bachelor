#include "draw.hpp"
#include "debug.hpp"
#include "assets.hpp"
#include "io.hpp"

#include <filesystem>
#include <unordered_map>

namespace no {

model::model(model&& that) : mesh(std::move(that.mesh)) {
	std::swap(root_transform, that.root_transform);
	std::swap(min_vertex, that.min_vertex);
	std::swap(max_vertex, that.max_vertex);
	std::swap(bones, that.bones);
	std::swap(nodes, that.nodes);
	std::swap(animations, that.animations);
	std::swap(drawable, that.drawable);
	std::swap(texture, that.texture);
}

model& model::operator=(model&& that) {
	std::swap(mesh, that.mesh);
	std::swap(root_transform, that.root_transform);
	std::swap(min_vertex, that.min_vertex);
	std::swap(max_vertex, that.max_vertex);
	std::swap(bones, that.bones);
	std::swap(nodes, that.nodes);
	std::swap(animations, that.animations);
	std::swap(drawable, that.drawable);
	std::swap(texture, that.texture);
	return *this;
}

int model::index_of_animation(const std::string& name) {
	for (int i = 0; i < (int)animations.size(); i++) {
		if (animations[i].name == name) {
			return i;
		}
	}
	return -1;
}

int model::total_animations() const {
	return (int)animations.size();
}

model_animation& model::animation(int index) {
	return animations[index];
}

model_node& model::node(int index) {
	return nodes[index];
}

int model::total_nodes() const {
	return (int)nodes.size();
}

glm::mat4 model::bone(int index) const {
	return bones[index];
}

void model::bind() const {
	mesh.bind();
}

void model::draw() const {
	mesh.draw();
}

bool model::is_drawable() const {
	return drawable && mesh.exists();
}

vector3f model::min() const {
	return min_vertex;
}

vector3f model::max() const {
	return max_vertex;
}

vector3f model::size() const {
	return max_vertex - min_vertex;
}

std::string model::texture_name() const {
	return texture;
}

std::string model::name() const {
	return model_name;
}

model_instance::model_instance(model& source) {
	set_source(source);
}

model_instance::model_instance(model_instance&& that) {
	std::swap(source, that.source);
	std::swap(bones, that.bones);
	std::swap(animations, that.animations);
	std::swap(attachments, that.attachments);
	std::swap(attachment_id_counter, that.attachment_id_counter);
	std::swap(is_new_loop, that.is_new_loop);
	std::swap(is_new_animation, that.is_new_animation);
	std::swap(new_animation_transition_frame, that.new_animation_transition_frame);
	std::swap(animation_index, that.animation_index);
	std::swap(animation_timer, that.animation_timer);
}

model_instance& model_instance::operator=(model_instance&& that) {
	std::swap(source, that.source);
	std::swap(bones, that.bones);
	std::swap(animations, that.animations);
	std::swap(attachments, that.attachments);
	std::swap(attachment_id_counter, that.attachment_id_counter);
	std::swap(is_new_loop, that.is_new_loop);
	std::swap(is_new_animation, that.is_new_animation);
	std::swap(new_animation_transition_frame, that.new_animation_transition_frame);
	std::swap(animation_index, that.animation_index);
	std::swap(animation_timer, that.animation_timer);
	return *this;
}

void model_instance::set_source(model& source) {
	this->source = &source;
	bones = source.bones;
	animation_timer.start();
	for (auto& animation : source.animations) {
		animations.emplace_back();
		for (auto& node : animation.channels) {
			animations.back().channels.emplace_back();
		}
	}
}

glm::mat4 model_instance::next_interpolated_position(int node_index, float time) {
	auto& node = source->animations[animation_index].channels[node_index];
	for (int p = animations[animation_index].channels[node_index].last_position_key; p < (int)node.positions.size() - 1; p++) {
		if (node.positions[p + 1].first > time) {
			animations[animation_index].channels[node_index].last_position_key = p;
			auto& current = node.positions[p];
			auto& next = node.positions[p + 1];
			float delta_time = next.first - current.first;
			float factor = (time - current.first) / delta_time;
			ASSERT(factor >= 0.0f && factor <= 1.0f);
			vector3f delta_position = next.second - current.second;
			vector3f interpolated_delta = factor * delta_position;
			vector3f translation = current.second + interpolated_delta;
			return glm::translate(glm::mat4(1.0f), { translation.x, translation.y, translation.z });
		}
	}
	if (node.positions.empty()) {
		return glm::mat4(1.0f);
	}
	auto& translation = node.positions.front().second;
	return glm::translate(glm::mat4(1.0f), { translation.x, translation.y, translation.z });
}

glm::mat4 model_instance::next_interpolated_rotation(int node_index, float time) {
	auto& node = source->animations[animation_index].channels[node_index];
	for (int r = animations[animation_index].channels[node_index].last_rotation_key; r < (int)node.rotations.size() - 1; r++) {
		if (node.rotations[r + 1].first > time) {
			animations[animation_index].channels[node_index].last_rotation_key = r;
			auto& current = node.rotations[r];
			auto& next = node.rotations[r + 1];
			float delta_time = next.first - current.first;
			float factor = (time - current.first) / delta_time;
			ASSERT(factor >= 0.0f && factor <= 1.0f);
			return glm::mat4_cast(glm::normalize(glm::slerp(current.second, next.second, factor)));
		}
	}
	if (node.rotations.empty()) {
		return glm::mat4(1.0f);
	}
	return glm::mat4_cast(node.rotations.front().second);
}

glm::mat4 model_instance::next_interpolated_scale(int node_index, float time) {
	auto& node = source->animations[animation_index].channels[node_index];
	for (int s = animations[animation_index].channels[node_index].last_scale_key; s < (int)node.scales.size() - 1; s++) {
		if (node.scales[s + 1].first > time) {
			animations[animation_index].channels[node_index].last_scale_key = s;
			auto& current = node.scales[s];
			auto& next = node.scales[s + 1];
			float delta_time = next.first - current.first;
			float factor = (time - current.first) / delta_time;
			ASSERT(factor >= 0.0f && factor <= 1.0f);
			vector3f delta_scale = next.second - current.second;
			vector3f interpolated_delta = factor * delta_scale;
			vector3f scale = current.second + interpolated_delta;
			return glm::scale(glm::mat4(1.0f), { scale.x, scale.y, scale.z });
		}
	}
	if (node.scales.empty()) {
		return glm::mat4(1.0f);
	}
	auto& scale = node.scales.front().second;
	return glm::scale(glm::mat4(1.0f), { scale.x, scale.y, scale.z });
}

void model_instance::animate_node(int node_index, float time, const glm::mat4& transform) {
	auto& state = animations[animation_index].channels[node_index];
	auto& animation = source->animations[animation_index];
	glm::mat4 node_transform = source->nodes[node_index].transform;
	auto& node = animation.channels[node_index];
	if (node.positions.size() > 0 || node.rotations.size() > 0 || node.scales.size() > 0) {
		if (is_new_loop) {
			state.last_position_key = new_animation_transition_frame;
			state.last_rotation_key = new_animation_transition_frame;
			state.last_scale_key = new_animation_transition_frame;
		}
		glm::mat4 translation = next_interpolated_position(node_index, time);
		glm::mat4 rotation = next_interpolated_rotation(node_index, time);
		glm::mat4 scale = next_interpolated_scale(node_index, time);
		node_transform = translation * rotation * scale;
	}
	glm::mat4 new_transform = transform * node_transform;
	if (node.bone != -1) {
		if (my_attachment) {
			bones[node.bone] = new_transform * my_attachment->parent_bone * my_attachment->attachment_bone * source->root_transform;
		} else {
			bones[node.bone] = source->root_transform * new_transform * source->bones[node.bone];
		}
	}
	auto& children = source->nodes[node_index].children;
	for (int child : children) {
		animate_node(child, time, new_transform);
	}
	for (auto& attachment : state.attachments) {
		attachment->attachment.animate(new_transform);
	}
}

void model_instance::animate() {
	animate(glm::mat4(1.0f));
}

void model_instance::animate(const glm::mat4& transform) {
	auto& animation = source->animations[animation_index];
	double seconds = (double)animation_timer.milliseconds() * 0.001;
	double play_duration = seconds * (double)animation.ticks_per_second;
	is_new_loop = (play_duration >= (double)animation.duration) || is_new_animation;
	is_new_animation = false;
	float animation_time = (float)std::fmod(play_duration, (double)animation.duration);
	animate_node(0, animation_time, transform);
	new_animation_transition_frame = 0;
}

void model_instance::start_animation(int index) {
	if (index == animation_index) {
		return;
	}
	if (index < 0 || index >= source->total_animations()) {
		index = 0; // avoid crash
		WARNING("Invalid animation index passed: " << index);
	}
	new_animation_transition_frame = source->animations[animation_index].transitions[index];
	animation_index = index;
	animation_timer.start();
	// todo: fix this up, since nodes[2] is not guaranteed to have frames
	//float seconds = source.animations[animation_index].nodes[2].positions[new_animation_transition_frame].first;
	//float ms = seconds * 1000.0f;
	//animation_timer.go_back_in_time((long long)ms);
	is_new_animation = true;
}

bool model_instance::can_animate() const {
	return source && (int)source->animations.size() > animation_index && animation_index >= 0;
}

void model_instance::draw() const {
	get_shader_variable("uni_Bones").set(bones);
	source->bind();
	source->draw();
	for (size_t node_index : attachments) {
		auto& node = animations[animation_index].channels[node_index];
		for (auto& attachment : node.attachments) {
			attachment->attachment.draw();
		}
	}
}

int model_instance::attach(model& attachment_model, model_attachment_mapping_list& mappings) {
	attachment_id_counter++;
	for (int i = 0; i < (int)animations.size(); i++) {
		auto attachment = new model_attachment();
		bool found = mappings.update(*source, source->animations[i], attachment_model, *attachment);
		if (!found) {
			delete attachment;
			continue;
		}
		attachment->id = attachment_id_counter;
		animations[i].channels[attachment->parent].attachments.push_back(attachment);
		attachment->attachment.my_attachment = attachment;
		attachments.push_back(attachment->parent);
	}
	return attachment_id_counter;
}

void model_instance::detach(int id) {
	for (auto& animation : animations) {
		for (auto& channel : animation.channels) {
			for (size_t ca = 0; ca < channel.attachments.size(); ca++) {
				if (channel.attachments[ca]->id == id) {
					for (int a = 0; a < (int)attachments.size(); a++) {
						if (attachments[a] == ca) {
							attachments.erase(attachments.begin() + a);
							a--;
						}
					}
					delete channel.attachments[ca];
					channel.attachments.erase(channel.attachments.begin() + ca);
				}
			}
		}
	}
}

void model_instance::set_attachment_bone(int id, const no::vector3f& position, const glm::quat& rotation) {
	for (auto& animation : animations) {
		for (auto& node : animation.channels) {
			for (auto& attachment : node.attachments) {
				if (attachment->id == id) {
					attachment->position = position;
					attachment->rotation = rotation;
					attachment->update_bone();
				}
			}
		}
	}
}

void model_instance::update_attachment_bone(int id, const model_attachment_mapping_list& mappings) {
	int i = 0;
	for (auto& animation : animations) {
		for (auto& node : animation.channels) {
			for (auto& attachment : node.attachments) {
				if (attachment->id == id) {
					mappings.update(*source, source->animations[i], *attachment->attachment.source, *attachment);
				}
			}
		}
		i++;
	}
}

model_attachment::model_attachment(model& attachment, glm::mat4 parent_bone, vector3f position, glm::quat rotation, int parent, int id)
	: attachment(attachment), parent_bone(parent_bone), position(position), rotation(rotation), parent(parent), id(id) {
	update_bone();
}

void model_attachment::update_bone() {
	glm::mat4 t = glm::translate(glm::mat4(1.0f), { position.x, position.y, position.z });
	glm::mat4 r = glm::mat4_cast(glm::normalize(rotation));
	attachment_bone = t * r;
}

bool model_attachment_mapping::is_same_mapping(const model_attachment_mapping& that) const {
	if (root_model != that.root_model) {
		return false;
	}
	if (attached_model != that.attached_model || attached_animation != that.attached_animation) {
		return false;
	}
	return true;
}

std::string model_attachment_mapping::mapping_string() const {
	return root_model + "." + root_animation + " -> " + attached_model + "." + attached_animation;
}

void model_attachment_mapping_list::save(const std::string& path) {
	no::io_stream stream;
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
	no::file::write(path, stream);
}

void model_attachment_mapping_list::load(const std::string& path) {
	no::io_stream stream;
	no::file::read(path, stream);
	if (stream.write_index() == 0) {
		return;
	}
	int32_t count = stream.read<int32_t>();
	for (int32_t i = 0; i < count; i++) {
		model_attachment_mapping mapping;
		mapping.root_model = stream.read<std::string>();
		mapping.root_animation = stream.read<std::string>();
		mapping.attached_model = stream.read<std::string>();
		mapping.attached_animation = stream.read<std::string>();
		mapping.attached_to_channel = stream.read<int32_t>();
		mapping.position = stream.read<no::vector3f>();
		mapping.rotation = stream.read<glm::quat>();
		mappings.push_back(mapping);
	}
}

void model_attachment_mapping_list::for_each(const std::function<bool(model_attachment_mapping&)>& handler) {
	for (auto& mapping : mappings) {
		if (!handler(mapping)) {
			break;
		}
	}
}

void model_attachment_mapping_list::remove_if(const std::function<bool(model_attachment_mapping&)>& compare) {
	for (int i = 0; i < (int)mappings.size(); i++) {
		if (compare(mappings[i])) {
			mappings.erase(mappings.begin() + i);
			i--;
		}
	}
}

bool model_attachment_mapping_list::exists(const model_attachment_mapping& other) {
	for (auto& mapping : mappings) {
		if (mapping.is_same_mapping(other) && mapping.root_animation == other.root_animation) {
			return true;
		}
	}
	return false;
}

void model_attachment_mapping_list::add(const model_attachment_mapping& mapping) {
	if (!exists(mapping)) {
		mappings.push_back(mapping);
	}
}

bool model_attachment_mapping_list::update(model& root, model_animation& animation, model& attachment_model, model_attachment& attachment) const {
	for (auto& mapping : mappings) {
		if (mapping.root_model != root.name()) {
			continue;
		}
		if (mapping.attached_model != attachment_model.name()) {
			continue;
		}
		if (mapping.root_animation != animation.name) {
			continue;
		}
		attachment.attachment = { attachment_model };
		attachment.parent = mapping.attached_to_channel;
		attachment.parent_bone = root.bone(animation.channels[attachment.parent].bone);
		attachment.position = mapping.position;
		attachment.rotation = mapping.rotation;
		attachment.update_bone();
		return true;
	}
	return false;
}

std::string model_attachment_mapping_list::find_root_animation(const std::string& root_model, const std::string& attached_model, const std::string& attached_animation) const {
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

rectangle::rectangle(float x, float y, float width, float height) {
	set_tex_coords(x, y, width, height);
}

rectangle::rectangle() {
	set_tex_coords(0.0f, 0.0f, 1.0f, 1.0f);
}

void rectangle::set_tex_coords(float x, float y, float width, float height) {
	vertices.set({
		{ { 0.0f, 0.0f }, 1.0f, { x, y } },
		{ { 1.0f, 0.0f }, 1.0f, { x + width, y } },
		{ { 1.0f, 1.0f }, 1.0f, { x + width, y + height } },
		{ { 0.0f, 1.0f }, 1.0f, { x, y + height } }
	}, { 0, 1, 2, 3, 2, 0 });
}

void rectangle::bind() const {
	vertices.bind();
}

void rectangle::draw() const {
	vertices.draw();
}

}

std::ostream& operator<<(std::ostream& out, no::swap_interval interval) {
	switch (interval) {
	case no::swap_interval::late: return out << "Late";
	case no::swap_interval::immediate: return out << "Immediate";
	case no::swap_interval::sync: return out << "Sync";
	default: return out << "Unknown";
	}
}
