#pragma once

#include "platform.hpp"
#include "transform.hpp"
#include "containers.hpp"

#include "timer.hpp"

#include <vector>

#include "glm/gtc/quaternion.hpp"

namespace no {

#define MEASURE_REDUNDANT_BIND_CALLS DEBUG_ENABLED

class surface;
class ortho_camera;
class perspective_camera;
class shader_variable;
class io_stream;
struct model_data;

enum class swap_interval { late, immediate, sync };
enum class scale_option { nearest_neighbour, linear };
enum class polygon_render_mode { fill, wireframe };

// todo: make this an "int_to_float" and "byte_to_int" etc. type enum instead?
enum class attribute_component { is_float, is_integer, is_byte };

struct vertex_attribute_specification {

	attribute_component type = attribute_component::is_float;
	int components = 0;
	bool normalized = false;

	constexpr vertex_attribute_specification(int components) : components(components) {}
	constexpr vertex_attribute_specification(attribute_component type, int components)
		: type(type), components(components) {}
	constexpr vertex_attribute_specification(attribute_component type, int components, bool normalized)
		: type(type), components(components), normalized(normalized) {}

};

using vertex_specification = std::vector<vertex_attribute_specification>;

int create_vertex_array(const vertex_specification& specification);
void bind_vertex_array(int id);
void set_vertex_array_vertices(int id, uint8_t* buffer, size_t size);
void set_vertex_array_indices(int id, uint8_t* buffer, size_t size);
void draw_vertex_array(int id);
void draw_vertex_array(int id, size_t offset, size_t count);
void delete_vertex_array(int id);

int create_texture();
int create_texture(const surface& surface, scale_option scaling, bool mipmap);
int create_texture(const surface& surface);
void bind_texture(int id);
void bind_texture(int id, int slot);
void load_texture(int id, const surface& surface, scale_option scaling, bool mipmap);
void load_texture(int id, const surface& surface);
void load_texture_from_screen(int id, int bottom_y, int x, int y, int width, int height);
vector2i texture_size(int id);
void delete_texture(int id);

int create_shader(const std::string& path);
void bind_shader(int id);
shader_variable get_shader_variable(const std::string& name);
void set_shader_model(const transform& transform);
void set_shader_view_projection(const ortho_camera& camera);
void set_shader_view_projection(const perspective_camera& camera);
void delete_shader(int id);

void set_polygon_render_mode(polygon_render_mode mode);

long total_redundant_bind_calls();

vector3i read_pixel_at(vector2i position);

struct model_importer_options {
	struct {
		bool create_default = false;
		std::string bone_name = "Bone";
	} bones;
};

// deals only with nom models
void export_model(const std::string& path, model_data& model);
void import_model(const std::string& path, model_data& model);

// exchange model format like COLLADA or OBJ to nom model format
void convert_model(const std::string& source, const std::string& destination, model_importer_options options);
void convert_model(const std::string& source, const std::string& destination);

// if multiple models have identical vertex data, they can be merged into one model with all animations
// source files must already be converted to nom format. validation is done during the merging process.
void merge_model_animations(const std::string& source_directory, const std::string& destination);

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

struct mesh_vertex {
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
// todo: have a bone buffer, which can be used to add 2 attributes?

struct height_map_vertex {
	static constexpr vertex_attribute_specification attributes[] = { 3, 2 };
	vector3f position;
	vector2f tex_coords;
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

	vertex_array_data() = default;
	vertex_array_data(const vertex_array_data&) = delete;
	vertex_array_data(vertex_array_data&& that) = default;

	vertex_array_data& operator=(const vertex_array_data&) = delete;

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

class shader_variable {
public:

	int location = -1;

	shader_variable() = default;
	shader_variable(unsigned int program_id, const std::string& name);

	void set(int value);
	void set(float value);
	void set(const vector2f& vector);
	void set(const vector3f& vector);
	void set(const vector4f& vector);
	void set(const glm::mat4& matrix);
	void set(const transform& transform);
	void set(vector2f* vector, size_t count);

	bool exists() const;

};

template<typename V>
class vertex_array {
public:

	vertex_array() {
		id = create_vertex_array(vertex_specification(std::begin(V::attributes), std::end(V::attributes)));
	}

	vertex_array(const std::vector<V>& vertices, const std::vector<unsigned short>& indices) {
		id = create_vertex_array(vertex_specification(std::begin(V::attributes), std::end(V::attributes)));
		set(vertices, indices);
	}

	vertex_array(const vertex_array<V>&) = delete;

	vertex_array(vertex_array<V>&& that) {
		std::swap(id, that.id);
	}

	~vertex_array() {
		delete_vertex_array(id);
	}

	vertex_array<V>& operator=(const vertex_array<V>&) = delete;

	vertex_array<V>& operator=(vertex_array<V>&& that) {
		std::swap(id, that.id);
		return *this;
	}

	void set_vertices(const std::vector<V>& vertices) {
		set_vertex_array_vertices(id, (uint8_t*)&vertices[0], vertices.size() * sizeof(V));
	}

	void set_vertices(uint8_t* vertices, size_t vertex_count) {
		set_vertex_array_vertices(id, vertices, vertex_count * sizeof(V));
	}

	void set_indices(const std::vector<unsigned short>& indices) {
		set_vertex_array_indices(id, (uint8_t*)&indices[0], indices.size() * sizeof(unsigned short));
	}

	void set_indices(uint8_t* indices, size_t index_count) {
		set_vertex_array_indices(id, indices, index_count * sizeof(unsigned short));
	}

	void set(const std::vector<V>& vertices, const std::vector<unsigned short>& indices) {
		set_vertices(vertices);
		set_indices(indices);
	}

	void set(uint8_t* vertices, size_t vertex_count, uint8_t* indices, size_t index_count) {
		set_vertices(vertices, vertex_count);
		set_indices(indices, index_count);
	}

	void bind() const {
		bind_vertex_array(id);
	}

	void draw() const {
		draw_vertex_array(id);
	}

	void draw(size_t offset, size_t count) const {
		draw_vertex_array(id, offset, count);
	}

private:

	 int id = -1;

};

struct model_animation_channel {

	using key_time = float;

	std::vector<std::pair<key_time, vector3f>> positions;
	std::vector<std::pair<key_time, glm::quat>> rotations;
	std::vector<std::pair<key_time, vector3f>> scales;
	
	int bone = -1;

};

struct model_animation {

	std::string name;
	float duration = 0.0f;
	float ticks_per_second = 0.0f;
	std::vector<model_animation_channel> channels;
	std::vector<int> transitions;

};

struct mesh_bone_weight {
	int vertex = 0;
	float weight = 0.0f;
};

struct model_node {
	std::string name;
	glm::mat4 transform;
	std::vector<int> children;
};

struct model_data {
	glm::mat4 transform;
	vertex_array_data<mesh_vertex> shape;
	std::vector<glm::mat4> bones;
	std::vector<model_node> nodes;
	std::vector<model_animation> animations;
};

class model {
public:

	friend class model_instance;

	model() = default;
	model(const model&) = delete;
	model(model&&);
	model(const std::string& path);

	~model() = default;

	model& operator=(const model&) = delete;
	model& operator=(model&&);

	int index_of_animation(const std::string& name);
	int total_animations() const;

	void load(const std::string& path);
	void draw() const;

private:

	vertex_array<mesh_vertex> mesh;
	glm::mat4 root_transform;
	std::vector<glm::mat4> bones;
	std::vector<model_node> nodes;
	std::vector<model_animation> animations;

};

struct model_attachment;

struct model_animation_channel_state {
	int last_position_key = 0;
	int last_rotation_key = 0;
	int last_scale_key = 0;
	std::vector<model_attachment> attachments;
};

struct model_animation_instance {
	std::vector<model_animation_channel_state> channels;
};

class model_instance {
public:

	model_instance(model& source);

	model_instance(const model_instance&) = delete;
	model_instance(model_instance&&);

	model_instance& operator=(const model_instance&) = delete;
	model_instance& operator=(model_instance&&);

	model& original() const;

	void animate();
	void start_animation(int index);

	void draw() const;

	int attach(int parent, model& attachment, vector3f position, glm::quat rotation);
	bool detach(int id);

private:

	glm::mat4 next_interpolated_position(int node, float time);
	glm::mat4 next_interpolated_rotation(int node, float time);
	glm::mat4 next_interpolated_scale(int node, float time);
	void animate_node(int node, float time, const glm::mat4& transform);
	void animate(const glm::mat4& transform);

	model& source;
	std::vector<glm::mat4> bones;
	std::vector<model_animation_instance> animations;

	std::vector<size_t> attachments;
	int attachment_id_counter = 0;

	// used to reset the animation nodes' last key indices
	bool is_new_loop = false;
	bool is_new_animation = false;
	// does not interpolate to the new animation yet. it skips to the frame immediately
	int new_animation_transition_frame = 0;

	int animation_index = 0;
	timer animation_timer;

	// todo: eh, refactor tbh
	model_attachment* my_attachment = nullptr;

};

struct model_attachment {

	model_instance attachment;
	glm::mat4 parent_bone;
	vector3f position;
	glm::quat rotation;
	glm::mat4 attachment_bone;
	int parent = 0;
	int id = 0;

	model_attachment(model& attachment, glm::mat4 parent_bone, vector3f position, glm::quat rotation, int parent, int id) 
		: attachment(attachment), parent_bone(parent_bone), position(position), rotation(rotation), parent(parent), id(id) {
		glm::mat4 t = glm::translate(glm::mat4(1.0f), { position.x, position.y, position.z });
		glm::mat4 r = glm::mat4_cast(glm::normalize(rotation));
		attachment_bone = t * r;
	}

};

class rectangle {
public:

	rectangle();

	void set_tex_coords(float x, float y, float width, float height);
	void bind() const;
	void draw() const;

private:

	vertex_array<sprite_vertex> vertices;

};

template<typename V>
class quad {
public:

	quad() {
		set({}, {}, {}, {});
	}
	
	quad(const V& top_left, const V& top_right, const V& bottom_left, const V& bottom_right) {
		set(top_left, top_right, bottom_right, bottom_left);
	}

	void set(const V& top_left, const V& top_right, const V& bottom_left, const V& bottom_right) {
		vertices.set({ top_left, top_right, bottom_right, bottom_left }, { 0, 1, 2, 3, 2, 0 });
	}

	void draw() const {
		vertices.bind();
		vertices.draw();
	}

private:

	vertex_array<V> vertices;

};

template<typename V>
class tiled_quad_array {
public:

	tiled_quad_array() = default;
	tiled_quad_array(const tiled_quad_array&) = delete;
	tiled_quad_array(tiled_quad_array&& that) : shape(std::move(that.shape)) {
		std::swap(size, that.size);
		std::swap(vertices, that.vertices);
		std::swap(indices, that.indices);
	}

	tiled_quad_array& operator=(const tiled_quad_array&) = delete;
	tiled_quad_array& operator=(tiled_quad_array&&) = delete;

	void resize_and_reset(vector2i size) {
		this->size = size;
		build_vertices();
		build_indices();
	}

	void bind() const {
		shape.bind();
	}

	void draw() const {
		shape.draw();
	}

	void refresh() {
		shape.set_vertices(vertices);
	}

	void set_y(const shifting_2d_array<float>& ys) {
		int i = 0;
		for (int y = 0; y < ys.rows(); y++) {
			for (int x = 0; x < ys.columns(); x++) {
				vertices[i].position.y = ys.at(x + ys.x(), y + ys.y());
				i++;
			}
		}
	}

	V& vertex(int x, int y) {
		return vertices[y * size.x + x];
	}

	int width() const {
		return size.x;
	}

	int height() const {
		return size.y;
	}

private:

	void build_vertices() {
		vertices.clear();
		vertices.insert(vertices.begin(), size.x * size.y, {});
		for (int x = 0; x < size.x; x++) {
			for (int y = 0; y < size.y; y++) {
				vertices[y * size.x + x].position = { (float)x, 0.0f, (float)y };
			}
		}
		shape.set_vertices(vertices);
	}

	void build_indices() {
		indices.clear();
		indices.reserve(size.x * size.y);
		for (int x = 0; x < size.x - 1; x++) {
			for (int y = 0; y < size.y - 1; y++) {
				int top_left = y * size.x + x;
				int top_right = y * size.x + x + 1;
				int bottom_right = (y + 1) * size.x + x + 1;
				int bottom_left = (y + 1) * size.x + x;
				indices.push_back(top_left);
				indices.push_back(top_right);
				indices.push_back(bottom_right);
				indices.push_back(top_left);
				indices.push_back(bottom_left);
				indices.push_back(bottom_right);
			}
		}
		shape.set_indices(indices);
	}

	vertex_array<V> shape;
	vector2i size;
	std::vector<V> vertices;
	std::vector<unsigned short> indices;

};

}

std::ostream& operator<<(std::ostream& out, no::swap_interval interval);
