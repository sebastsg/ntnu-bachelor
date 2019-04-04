#include "draw.hpp"
#include "debug.hpp"
#include "assets.hpp"
#include "io.hpp"

#include <filesystem>
#include <unordered_map>

namespace no {

generic_vertex_array::generic_vertex_array(generic_vertex_array&& that) {
	std::swap(id, that.id);
}

generic_vertex_array::~generic_vertex_array() {
	delete_vertex_array(id);
}

generic_vertex_array& generic_vertex_array::operator=(generic_vertex_array&& that) {
	std::swap(id, that.id);
	return *this;
}

void generic_vertex_array::bind() const {
	bind_vertex_array(id);
}

void generic_vertex_array::draw() const {
	draw_vertex_array(id);
}

void generic_vertex_array::draw(size_t offset, size_t count) const {
	draw_vertex_array(id, offset, count);
}

bool generic_vertex_array::exists() const {
	return id != -1;
}

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

int model::index_of_animation(const std::string& name) const {
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

const model_animation& model::animation(int index) const {
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

void sprite_animation::update(float delta) {
	if (paused) {
		return;
	}
	previous_frame = current_frame;
	sub_frame += fps * delta;
	current_frame = (int)sub_frame;
	if (current_frame >= frames) {
		current_frame = 0;
		sub_frame = 0.0f;
	}
	if (previous_frame != current_frame) {
		set_tex_coords(uv_position, uv_size);
	}
}

void sprite_animation::draw(vector2f position, vector2f size) const {
	draw_shape(rectangle, transform2{ position, size });
}

void sprite_animation::draw(vector2f position, int texture) const {
	draw_shape(rectangle, transform2{ position, texture_size(texture).to<float>() });
}

void sprite_animation::draw(const transform2& transform) const {
	draw_shape(rectangle, transform);
}

void sprite_animation::pause() {
	paused = true;
}

void sprite_animation::resume() {
	paused = false;
}

bool sprite_animation::is_paused() const {
	return paused;
}

void sprite_animation::set_frame(int frame) {
	previous_frame = current_frame;
	current_frame = frame;
	sub_frame = (float)frame;
	set_tex_coords(uv_position, uv_size);
}

void sprite_animation::set_tex_coords(vector2f position, vector2f size) {
	uv_position = position;
	uv_size = size;
	const float frame_width = uv_size.x / (float)frames;
	rectangle.set_tex_coords(uv_position.x + frame_width * (float)current_frame, uv_position.y, frame_width, uv_size.y);
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
