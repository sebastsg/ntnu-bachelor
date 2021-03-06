#pragma once

#include "platform.hpp"

#if ENABLE_GRAPHICS

#include "transform.hpp"
#include "io.hpp"
#include "debug.hpp"

#include "glm/gtc/quaternion.hpp"

#include <functional>

namespace no {

// todo: make this an "int_to_float" and "byte_to_int" etc. type enum instead?
enum class attribute_component { is_float, is_integer, is_byte };

struct vertex_attribute_specification {

	attribute_component type = attribute_component::is_float;
	int components = 0;
	bool normalized = false;

	constexpr vertex_attribute_specification(int components) : components(components) {}
	constexpr vertex_attribute_specification(attribute_component type, int components)
		: type(type), components(components) {
	}
	constexpr vertex_attribute_specification(attribute_component type, int components, bool normalized)
		: type(type), components(components), normalized(normalized) {
	}

};

using vertex_specification = std::vector<vertex_attribute_specification>;

struct tiny_sprite_vertex {
	static constexpr vertex_attribute_specification attributes[] = { 2, 2 };
	vector2f position;
	vector2f tex_coords;
};

struct sprite_vertex {
	static constexpr vertex_attribute_specification attributes[] = { 2, 4, 2 };
	vector2f position;
	vector4f color = 1.0f;
	vector2f tex_coords;
};

struct static_mesh_vertex {
	static constexpr vertex_attribute_specification attributes[] = { 3, 3, 2 };
	vector3f position;
	vector3f color = 1.0f;
	vector2f tex_coords;
};

struct animated_mesh_vertex {
	static constexpr vertex_attribute_specification attributes[] = {
		3, 4, 2, 3, 3, 3, 4, 2, { attribute_component::is_integer, 4 }, { attribute_component::is_integer, 2 }
	};
	vector3f position;
	vector4f color = 1.0f;
	vector2f tex_coords;
	vector3f normal;
	vector3f tangent;
	vector3f bitangent;
	vector4f weights;
	vector2f weights_extra;
	vector4i bones;
	vector2i bones_extra;
};

struct pick_vertex {
	static constexpr vertex_attribute_specification attributes[] = { 3, 3 };
	vector3f position;
	vector3f color;
};

template<typename V>
struct vertex_array_data {

	std::vector<V> vertices;
	std::vector<unsigned short> indices;

	bool operator==(const vertex_array_data& that) const {
		if (vertices.size() != that.vertices.size()) {
			return false;
		}
		if (indices.size() != that.indices.size()) {
			return false;
		}
		for (size_t i = 0; i < vertices.size(); i++) {
			if (memcmp(&vertices[i], &that.vertices[i], sizeof(V))) {
				return false;
			}
		}
		for (size_t i = 0; i < indices.size(); i++) {
			if (indices[i] != that.indices[i]) {
				return false;
			}
		}
		return true;
	}

	bool operator!=(const vertex_array_data& that) const {
		return !operator==(that);
	}

};

struct animation_channel {

	using key_time = float;

	struct position_frame {
		key_time time = 0.0f;
		vector3f position;
		position_frame() = default;
		position_frame(key_time time, vector3f position);
	};

	struct rotation_frame {
		key_time time = 0.0f;
		glm::quat rotation;
		rotation_frame() = default;
		rotation_frame(key_time time, glm::quat rotation);
	};

	struct scale_frame {
		key_time time = 0.0f;
		vector3f scale;
		scale_frame() = default;
		scale_frame(key_time time, vector3f scale);
	};

	std::vector<position_frame> positions;
	std::vector<rotation_frame> rotations;
	std::vector<scale_frame> scales;

	int bone = -1;

};

struct model_animation {
	std::string name;
	float duration = 0.0f;
	float ticks_per_second = 0.0f;
	std::vector<animation_channel> channels;
	std::vector<int> transitions;
};

struct model_node {
	std::string name;
	glm::mat4 transform;
	std::vector<int> children;
	int depth = 0;
};

template<typename V>
struct model_data {
	glm::mat4 transform;
	vector3f min;
	vector3f max;
	vertex_array_data<V> shape;
	std::vector<std::string> bone_names;
	std::vector<glm::mat4> bones;
	std::vector<model_node> nodes;
	std::vector<model_animation> animations;
	std::string texture;
	std::string name;

	template<typename T>
	model_data<T> to(const std::function<T(V)>& mapper) const {
		model_data<T> that;
		that.transform = transform;
		that.min = min;
		that.max = max;
		for (auto& vertex : shape.vertices) {
			that.shape.vertices.push_back(mapper(vertex));
		}
		that.shape.indices = shape.indices;
		that.bone_names = bone_names;
		that.bones = bones;
		that.nodes = nodes;
		that.animations = animations;
		that.texture = texture;
		that.name = name;
		return that;
	}

};

template<typename V>
model_data<V> create_box_model_data(const std::function<V(const vector3f&)>& mapper) {
	model_data<V> data;
	data.name = "box";
	data.min = 0.0f;
	data.max = 1.0f;
	const vector3f vertices[] = {
		{ 0.0f, 0.0f, 0.0f }, // (top left) lower - 0
		{ 0.0f, 1.0f, 0.0f }, // (top left) upper - 1
		{ 1.0f, 0.0f, 0.0f }, // (top right) lower - 2
		{ 1.0f, 1.0f, 0.0f }, // (top right) upper - 3
		{ 0.0f, 0.0f, 1.0f }, // (bottom left) lower - 4
		{ 0.0f, 1.0f, 1.0f }, // (bottom left) upper - 5
		{ 1.0f, 0.0f, 1.0f }, // (bottom right) lower - 6
		{ 1.0f, 1.0f, 1.0f }, // (bottom right) upper - 7
	};
	for (auto& vertex : vertices) {
		data.shape.vertices.push_back(mapper(vertex));
	}
	data.shape.indices = {
		0, 1, 5, 5, 4, 0, // left
		0, 1, 3, 3, 2, 0, // back
		2, 3, 7, 7, 6, 2, // right
		5, 7, 6, 6, 4, 5, // front
		0, 2, 6, 6, 4, 0, // bottom
		1, 3, 7, 7, 5, 1, // top
	};
	return data;
}

struct model_import_options {
	struct {
		bool create_default = false;
		std::string bone_name = "Bone";
	} bones;
};

struct model_conversion_options {
	model_import_options import;
	std::function<void(const std::string&, const model_data<animated_mesh_vertex>&)> exporter;
};

template<typename V>
void export_model(const std::string& path, const model_data<V>& model) {
	io_stream stream;
	stream.write(model.transform);
	stream.write(model.min);
	stream.write(model.max);
	stream.write(model.texture);
	stream.write(model.name);
	stream.write((int32_t)sizeof(V));
	stream.write_array<V>(model.shape.vertices);
	stream.write_array<uint16_t>(model.shape.indices);
	stream.write_array<std::string>(model.bone_names);
	stream.write_array<glm::mat4>(model.bones);
	stream.write((int16_t)model.nodes.size());
	for (auto& node : model.nodes) {
		stream.write(node.name);
		stream.write(node.transform);
		stream.write_array<int16_t, int>(node.children);
	}
	stream.write((int16_t)model.animations.size());
	for (auto& animation : model.animations) {
		stream.write(animation.name);
		stream.write(animation.duration);
		stream.write(animation.ticks_per_second);
		stream.write((int16_t)animation.channels.size());
		for (auto& node : animation.channels) {
			stream.write((int16_t)node.bone);
			stream.write((int16_t)node.positions.size());
			for (auto& position : node.positions) {
				stream.write(position.time);
				stream.write(position.position);
			}
			stream.write((int16_t)node.rotations.size());
			for (auto& rotation : node.rotations) {
				stream.write(rotation.time);
				stream.write(rotation.rotation);
			}
			stream.write((int16_t)node.scales.size());
			for (auto& scale : node.scales) {
				stream.write(scale.time);
				stream.write(scale.scale);
			}
		}
		stream.write_array<int16_t, int>(animation.transitions);
	}
	file::write(path, stream);
}

template<typename V>
void import_model(const std::string& path, model_data<V>& model) {
	io_stream stream;
	file::read(path, stream);
	if (stream.write_index() == 0) {
		WARNING("Failed to open file: " << path);
		return;
	}
	model.transform = stream.read<glm::mat4>();
	model.min = stream.read<vector3f>();
	model.max = stream.read<vector3f>();
	model.texture = stream.read<std::string>();
	model.name = stream.read<std::string>();
	int32_t vertex_size = stream.read<int32_t>();
	if (vertex_size != sizeof(V)) {
		WARNING(vertex_size << " != " << sizeof(V) << ". File: " << path);
		return;
	}
	model.shape.vertices = stream.read_array<V>();
	model.shape.indices = stream.read_array<uint16_t>();
	model.bone_names = stream.read_array<std::string>();
	model.bones = stream.read_array<glm::mat4>();
	int16_t node_count = stream.read<int16_t>();
	for (int16_t n = 0; n < node_count; n++) {
		auto& node = model.nodes.emplace_back();
		node.name = stream.read<std::string>();
		node.transform = stream.read<glm::mat4>();
		node.children = stream.read_array<int, int16_t>();
	}
	int16_t animation_count = stream.read<int16_t>();
	for (int16_t a = 0; a < animation_count; a++) {
		auto& animation = model.animations.emplace_back();
		animation.name = stream.read<std::string>();
		animation.duration = stream.read<float>();
		animation.ticks_per_second = stream.read<float>();
		int16_t node_count = stream.read<int16_t>();
		for (int16_t n = 0; n < node_count; n++) {
			auto& node = animation.channels.emplace_back();
			node.bone = (int)stream.read<int16_t>();
			int16_t position_count = stream.read<int16_t>();
			for (int16_t p = 0; p < position_count; p++) {
				auto& position = node.positions.emplace_back();
				position.time = stream.read<float>();
				position.position = stream.read<vector3f>();
			}
			int16_t rotation_count = stream.read<int16_t>();
			for (int16_t r = 0; r < rotation_count; r++) {
				auto& rotation = node.rotations.emplace_back();
				rotation.time = stream.read<float>();
				rotation.rotation = stream.read<glm::quat>();
			}
			int16_t scale_count = stream.read<int16_t>();
			for (int16_t s = 0; s < scale_count; s++) {
				auto& scale = node.scales.emplace_back();
				scale.time = stream.read<float>();
				scale.scale = stream.read<vector3f>();
			}
		}
		animation.transitions = stream.read_array<int, int16_t>();
	}
}

// if multiple models have identical vertex data, they can be merged into one model with all animations
// source files must already be converted to nom format. validation is done during the merging process.
template<typename V>
model_data<V> merge_model_animations(const std::vector<model_data<V>>& models) {
	if (models.empty()) {
		return {};
	}
	if (models.size() == 1) {
		return models.front();
	}
	model_data<V> output = models.front();
	for (size_t m = 1; m < models.size(); m++) {
		auto& model = models[m];
		if (model.transform != output.transform) {
			WARNING("Root transform not identical. Not skipped.");
		}
		if (output.nodes.size() != model.nodes.size()) {
			WARNING("Different number of node transforms. Skipping.");
			continue;
		}
		bool equal = true;
		for (size_t n = 0; n < output.nodes.size(); n++) {
			// maybe this isn't that important?
			/*if (output.nodes[n].transform != model.nodes[n].transform) {
				WARNING("Different node transform " << n << ". Skipping.");
				equal = false;
				break;
			}*/
			if (output.nodes[n].children != model.nodes[n].children) {
				WARNING("Different node children " << n << ". Skipping.");
				equal = false;
				break;
			}
		}
		if (!equal) {
			continue;
		}
		if (model.bones.size() != output.bones.size()) {
			WARNING("Mesh not identical. Skipping.");
			//continue;
		}
		for (size_t i = 0; i < model.bones.size(); i++) {
			if (model.bones[i] != output.bones[i]) {
				WARNING("Mesh not identical. Skipping.");
				//continue;
			}
		}
		if (model.shape != output.shape) {
			WARNING("Mesh not identical. Skipping.");
			//continue;
		}
		for (auto& animation : model.animations) {
			bool skip = false;
			for (auto& existing_animation : output.animations) {
				if (animation.name == existing_animation.name) {
					WARNING(animation.name << " already exists. Skipping animation.");
					skip = true;
					continue;
				}
			}
			if (skip) {
				continue;
			}
			output.animations.push_back(animation);
		}
	}
	std::vector<int> default_transitions;
	default_transitions.insert(default_transitions.begin(), output.animations.size(), 0);
	for (auto& animation : output.animations) {
		animation.transitions = default_transitions;
	}
	output.name = "";
	return output;
}


#if ENABLE_ASSIMP
void convert_model(const std::string& source, const std::string& destination, model_conversion_options options);
#endif

transform3 load_model_bounding_box(const std::string& path);

}

#endif
