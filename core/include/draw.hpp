#pragma once

#include "platform.hpp"
#include "transform.hpp"
#include "containers.hpp"
#include "timer.hpp"
#include "vertex.hpp"

namespace no {

#define MEASURE_REDUNDANT_BIND_CALLS DEBUG_ENABLED

class surface;
class ortho_camera;
class perspective_camera;
class shader_variable;

enum class swap_interval { late, immediate, sync };
enum class scale_option { nearest_neighbour, linear };
enum class polygon_render_mode { fill, wireframe };

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
void set_shader_model(const transform2& transform);
void set_shader_model(const transform3& transform);
void set_shader_view_projection(const ortho_camera& camera);
void set_shader_view_projection(const perspective_camera& camera);
void delete_shader(int id);

void set_polygon_render_mode(polygon_render_mode mode);

long total_redundant_bind_calls();

vector3i read_pixel_at(vector2i position);

class shader_variable {
public:

	int location = -1;

	shader_variable() = default;
	shader_variable(unsigned int program_id, const std::string& name);

	// debatable whether or not these should be const
	// it's useful for const draw functions
	void set(int value) const;
	void set(float value) const;
	void set(const vector2f& vector) const;
	void set(const vector3f& vector) const;
	void set(const vector4f& vector) const;
	void set(const glm::mat4& matrix) const;
	void set(const transform2& transform) const;
	void set(const transform3& transform) const;
	void set(vector2f* vector, size_t count) const;
	void set(const std::vector<glm::mat4>& matrices) const;

	bool exists() const;

};

template<typename V>
class vertex_array {
public:

	friend class generic_vertex_array;

	vertex_array() {
		id = create_vertex_array(vertex_specification(std::begin(V::attributes), std::end(V::attributes)));
	}

	vertex_array(const std::vector<V>& vertices, const std::vector<unsigned short>& indices) : vertex_array() {
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

class generic_vertex_array {
public:

	generic_vertex_array() = default;
	generic_vertex_array(const generic_vertex_array&) = delete;
	generic_vertex_array(generic_vertex_array&& that) {
		std::swap(id, that.id);
	}

	template<typename V>
	generic_vertex_array(vertex_array<V>&& that) {
		std::swap(id, that.id);
	}

	~generic_vertex_array() {
		delete_vertex_array(id);
	}

	generic_vertex_array& operator=(const generic_vertex_array&) = delete;
	generic_vertex_array& operator=(generic_vertex_array&& that) {
		std::swap(id, that.id);
		return *this;
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

	bool exists() const {
		return id != -1;
	}

private:

	int id = -1;

};

class model_attachment_mapping_list;

class model {
public:

	friend class model_instance;

	model() = default;
	model(const model&) = delete;
	model(model&&);

	~model() = default;

	model& operator=(const model&) = delete;
	model& operator=(model&&);

	int index_of_animation(const std::string& name);
	int total_animations() const;
	model_animation& animation(int index);
	model_node& node(int index);
	int total_nodes() const;
	glm::mat4 bone(int index) const;

	template<typename V>
	void load(const model_data<V>& model) {
		if (model.shape.vertices.empty()) {
			WARNING("Failed to load model");
			return;
		}
		mesh = { std::move(vertex_array<V>{model.shape.vertices, model.shape.indices }) };
		root_transform = model.transform;
		min_vertex = model.min;
		max_vertex = model.max;
		nodes = model.nodes;
		bones = model.bones;
		animations = model.animations;
		texture = model.texture;
		model_name = model.name;
		size_t vertices = model.shape.vertices.size();
		size_t indices = model.shape.indices.size();
		drawable = (vertices > 0 && indices > 0);
	}

	template<typename V>
	void load(const std::string& path) {
		model_data<V> model;
		import_model(path, model);
		if (model.shape.vertices.empty()) {
			WARNING("Failed to load model: " << path);
			return;
		}
		load(model);
	}
	
	void bind() const;
	void draw() const;

	bool is_drawable() const;

	vector3f min() const;
	vector3f max() const;
	vector3f size() const;

	std::string texture_name() const;
	std::string name() const;

private:

	generic_vertex_array mesh;
	glm::mat4 root_transform;
	std::vector<glm::mat4> bones;
	std::vector<model_node> nodes;
	std::vector<model_animation> animations;
	vector3f min_vertex;
	vector3f max_vertex;
	bool drawable = false;
	std::string texture;
	std::string model_name;

};

struct model_attachment;

struct model_animation_channel_state {
	int last_position_key = 0;
	int last_rotation_key = 0;
	int last_scale_key = 0;
	std::vector<model_attachment*> attachments;
};

struct model_animation_instance {
	std::vector<model_animation_channel_state> channels;
};

class model_instance {
public:

	model_instance() = default;
	model_instance(model& source);
	model_instance(const model_instance&) = delete;
	model_instance(model_instance&&);

	model_instance& operator=(const model_instance&) = delete;
	model_instance& operator=(model_instance&&);

	void set_source(model& source);

	void animate();
	void start_animation(int index);
	bool can_animate() const;

	void draw() const;

	int attach(model& attachment, model_attachment_mapping_list& mappings);
	void detach(int id);
	void set_attachment_bone(int id, const no::vector3f& position, const glm::quat& rotation);
	void update_attachment_bone(int id, const model_attachment_mapping_list& mappings);

private:

	glm::mat4 next_interpolated_position(int node, float time);
	glm::mat4 next_interpolated_rotation(int node, float time);
	glm::mat4 next_interpolated_scale(int node, float time);
	void animate_node(int node, float time, const glm::mat4& transform);
	void animate(const glm::mat4& transform);

	model* source = nullptr;
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
	int parent = -1;
	int id = 0;

	model_attachment() = default;
	model_attachment(model& attachment, glm::mat4 parent_bone, vector3f position, glm::quat rotation, int parent, int id);

	void update_bone();

};

struct model_attachment_mapping {
	std::string root_model;
	std::string root_animation;
	std::string attached_model;
	std::string attached_animation;
	int attached_to_channel = -1;
	vector3f position;
	glm::quat rotation;

	bool is_same_mapping(const model_attachment_mapping& that) const;
	std::string mapping_string() const;

};

class model_attachment_mapping_list {
public:

	void save(const std::string& path);
	void load(const std::string& path);

	void for_each(const std::function<bool(model_attachment_mapping&)>& handler);
	void remove_if(const std::function<bool(model_attachment_mapping&)>& compare);

	bool exists(const model_attachment_mapping& mapping);
	void add(const model_attachment_mapping& mapping);
	bool update(model& root, model_animation& animation, model& attachment_model, model_attachment& attachment) const;
	
	std::string find_root_animation(const std::string& root_model, const std::string& attached_model, const std::string& attached_animation) const;

private:

	std::vector<model_attachment_mapping> mappings;

};

class rectangle {
public:

	rectangle(float x, float y, float width, float height);
	rectangle();

	void set_tex_coords(float x, float y, float width, float height);
	void bind() const;
	void draw() const;

private:

	vertex_array<sprite_vertex> vertices;

};

class sprite_animation {
public:

	int frames = 1;
	float fps = 10.0f;

	void update(float delta);
	void draw(vector2f position, vector2f size) const;
	void draw(vector2f position, int texture) const;
	void draw(const transform2& transform) const;

	void pause();
	void resume();
	bool is_paused() const;
	void set_frame(int frame);

private:

	void set_tex_coords();

	rectangle rectangle;
	int current_frame = 0;
	float sub_frame = 0.0f;
	int previous_frame;
	bool paused = false;

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

	void bind() const {
		vertices.bind();
	}

	void draw() const {
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
		std::swap(quad_count, that.quad_count);
		std::swap(per_quad, that.per_quad);
		std::swap(vertices, that.vertices);
		std::swap(indices, that.indices);
	}

	tiled_quad_array& operator=(const tiled_quad_array&) = delete;
	tiled_quad_array& operator=(tiled_quad_array&&) = delete;

	void build(int vertices_per_quad, vector2i size, const std::function<void(int, int, std::vector<V>&, std::vector<unsigned short>&)>& builder) {
		per_quad = vertices_per_quad;
		quad_count = size;
		vertices.clear();
		vertices.reserve(size.x * size.y);
		indices.clear();
		indices.reserve(size.x * size.y);
		for (int y = 0; y < size.y; y++) {
			for (int x = 0; x < size.x; x++) {
				builder(x, y, vertices, indices);
			}
		}
		shape.set(vertices, indices);
	}

	void for_each(const std::function<void(int, int, int, std::vector<V>&)>& function) {
		int i = 0;
		for (int y = 0; y < quad_count.y; y++) {
			for (int x = 0; x < quad_count.x; x++) {
				function(i, x, y, vertices);
				i += per_quad;
			}
		}
	}

	void refresh() {
		shape.set_vertices(vertices);
	}

	void bind() const {
		shape.bind();
	}

	void draw() const {
		shape.draw();
	}

	int width() const {
		return quad_count.x;
	}

	int height() const {
		return quad_count.y;
	}

private:

	vertex_array<V> shape;
	vector2i quad_count;
	std::vector<V> vertices;
	std::vector<unsigned short> indices;
	int per_quad = 1;

};

template<typename T, typename M>
void draw_shape(const T& shape, const M& transform) {
	set_shader_model(transform);
	shape.bind();
	shape.draw();
}

}

std::ostream& operator<<(std::ostream& out, no::swap_interval interval);
