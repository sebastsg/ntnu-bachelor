#include "platform.hpp"

#if ENABLE_GL

#include <glew/glew.h>

#include "surface.hpp"
#include "io.hpp"
#include "transform.hpp"
#include "camera.hpp"
#include "debug.hpp"
#include "draw.hpp"

#include "GLM/gtc/type_ptr.hpp"

namespace no {

static int gl_pixel_format(pixel_format format) {
	switch (format) {
	case pixel_format::rgba: return GL_RGBA;
	case pixel_format::bgra: return GL_BGRA;
	default: return GL_RGBA;
	}
}

static int gl_scale_option(scale_option scaling, bool mipmap) {
	switch (scaling) {
	case scale_option::nearest_neighbour: return mipmap ? GL_NEAREST_MIPMAP_NEAREST : GL_NEAREST;
	case scale_option::linear: return mipmap ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR;
	default: return GL_NEAREST;
	}
}

struct gl_buffer {
	unsigned int id = 0;
	size_t allocated = 0;
	bool exists = false;
};

struct gl_vertex_array {
	unsigned int id = 0;
	int draw_mode = GL_TRIANGLES;
	gl_buffer vertex_buffer;
	gl_buffer index_buffer;
	unsigned int indices = 0;
};

struct gl_texture {
	unsigned int id = 0;
	vector2i size;
};

struct gl_shader {
	unsigned int id = 0;
	glm::mat4 model = glm::mat4(1.0f);
	glm::mat4 view;
	glm::mat4 projection;
	int model_view_projection_location = -1;
	int model_location = -1;
};

static struct {

	std::vector<gl_vertex_array> vertex_arrays;
	std::vector<gl_texture> textures;
	std::vector<gl_shader> shaders;

	int bound_shader = 0;
	
	long total_redundant_bind_calls = 0;
	
} renderer;

long total_redundant_bind_calls() {
	return renderer.total_redundant_bind_calls;
}

static size_t size_of_attribute_component(attribute_component type) {
	switch (type) {
	case attribute_component::is_float: return sizeof(float);
	case attribute_component::is_integer: return sizeof(int);
	case attribute_component::is_byte: return sizeof(uint8_t);
	default: return sizeof(float);
	}
}

int create_vertex_array(const vertex_specification& specification) {
	int id = -1;
	for (int i = 0; i < (int)renderer.vertex_arrays.size(); i++) {
		if (renderer.vertex_arrays[i].id == 0) {
			id = i;
			break;
		}
	}
	if (id == -1) {
		renderer.vertex_arrays.emplace_back();
		id = (int)renderer.vertex_arrays.size() - 1;
	}
	auto& vertex_array = renderer.vertex_arrays[id];
	CHECK_GL_ERROR(glGenVertexArrays(1, &vertex_array.id));
	CHECK_GL_ERROR(glGenBuffers(1, &vertex_array.vertex_buffer.id));
	CHECK_GL_ERROR(glGenBuffers(1, &vertex_array.index_buffer.id));
	bind_vertex_array(id);
	int vertex_size = 0;
	for (auto& attribute : specification) {
		vertex_size += attribute.components * size_of_attribute_component(attribute.type);
	}
	char* attribute_pointer = nullptr;
	for (size_t i = 0; i < specification.size(); i++) {
		auto& attribute = specification[i];
		int normalized = (attribute.normalized ? GL_TRUE : GL_FALSE);
		switch (attribute.type) {
		case attribute_component::is_float:
			CHECK_GL_ERROR(glVertexAttribPointer(i, attribute.components, GL_FLOAT, normalized, vertex_size, attribute_pointer));
			break;
		case attribute_component::is_integer:
			CHECK_GL_ERROR(glVertexAttribIPointer(i, attribute.components, GL_INT, vertex_size, attribute_pointer));
			break;
		case attribute_component::is_byte:
			CHECK_GL_ERROR(glVertexAttribPointer(i, attribute.components, GL_UNSIGNED_BYTE, normalized, vertex_size, attribute_pointer));
			break;
		}
		CHECK_GL_ERROR(glEnableVertexAttribArray(i));
		attribute_pointer += attribute.components * size_of_attribute_component(attribute.type);
	}
	return id;
}

void bind_vertex_array(int id) {
	const auto& vertex_array = renderer.vertex_arrays[id];
	CHECK_GL_ERROR(glBindVertexArray(vertex_array.id));
	CHECK_GL_ERROR(glBindBuffer(GL_ARRAY_BUFFER, vertex_array.vertex_buffer.id));
	CHECK_GL_ERROR(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vertex_array.index_buffer.id));
}

void set_vertex_array_vertices(int id, uint8_t* data, size_t size) {
	ASSERT(data && size > 0);
	auto& vertex_array = renderer.vertex_arrays[id];
	CHECK_GL_ERROR(glBindBuffer(GL_ARRAY_BUFFER, vertex_array.vertex_buffer.id));
	if (vertex_array.vertex_buffer.exists && vertex_array.vertex_buffer.allocated >= size) {
		CHECK_GL_ERROR(glBufferSubData(GL_ARRAY_BUFFER, 0, size, data));
	} else {
		CHECK_GL_ERROR(glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW));
		vertex_array.vertex_buffer.exists = true;
		vertex_array.vertex_buffer.allocated = size;
	}
}

void set_vertex_array_indices(int id, uint8_t* data, size_t size) {
	ASSERT(data && size > 0);
	auto& vertex_array = renderer.vertex_arrays[id];
	CHECK_GL_ERROR(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vertex_array.index_buffer.id));
	if (vertex_array.index_buffer.exists && vertex_array.index_buffer.allocated >= size) {
		CHECK_GL_ERROR(glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, size, data));
	} else {
		CHECK_GL_ERROR(glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, data, GL_STATIC_DRAW));
		vertex_array.index_buffer.exists = true;
		vertex_array.index_buffer.allocated = size;
	}
	vertex_array.indices = size / sizeof(unsigned short);
}

void draw_vertex_array(int id) {
	const auto& vertex_array = renderer.vertex_arrays[id];
	CHECK_GL_ERROR(glDrawElements(vertex_array.draw_mode, vertex_array.indices, GL_UNSIGNED_SHORT, nullptr));
}

void draw_vertex_array(int id, size_t offset, size_t count) {
	offset *= sizeof(unsigned short);
	const auto& vertex_array = renderer.vertex_arrays[id];
	CHECK_GL_ERROR(glDrawElements(vertex_array.draw_mode, count, GL_UNSIGNED_SHORT, (void*)offset));
}

void delete_vertex_array(int id) {
	if (id < 0) {
		return;
	}
	auto& vertex_array = renderer.vertex_arrays[id];
	CHECK_GL_ERROR(glDeleteVertexArrays(1, &vertex_array.id));
	CHECK_GL_ERROR(glDeleteBuffers(1, &vertex_array.vertex_buffer.id));
	CHECK_GL_ERROR(glDeleteBuffers(1, &vertex_array.index_buffer.id));
	vertex_array = {};
}

int create_texture() {
	int id = -1;
	for (int i = 0; i < (int)renderer.textures.size(); i++) {
		if (renderer.textures[i].id == 0) {
			id = i;
			break;
		}
	}
	if (id == -1) {
		renderer.textures.emplace_back();
		id = (int)renderer.textures.size() - 1;
	}
	auto& texture = renderer.textures[id];
	CHECK_GL_ERROR(glGenTextures(1, &texture.id));
	return id;
}

int create_texture(const surface& surface, scale_option scaling, bool mipmaps) {
	int id = create_texture();
	load_texture(id, surface, scaling, mipmaps);
	return id;
}

int create_texture(const surface& surface) {
	int id = create_texture();
	load_texture(id, surface);
	return id;
}

#if MEASURE_REDUNDANT_BIND_CALLS
void measure_redundant_bind_call(int id) {
	int active_texture_id = 0;
	CHECK_GL_ERROR(glGetIntegerv(GL_TEXTURE_BINDING_2D, &active_texture_id));
	if (active_texture_id == renderer.textures[id].id) {
		renderer.total_redundant_bind_calls++;
	}
}
#endif

void bind_texture(int id) {
#if MEASURE_REDUNDANT_BIND_CALLS
	measure_redundant_bind_call(id);
#endif
	CHECK_GL_ERROR(glBindTexture(GL_TEXTURE_2D, renderer.textures[id].id));
}

void bind_texture(int id, int slot) {
	CHECK_GL_ERROR(glActiveTexture(GL_TEXTURE0 + slot));
#if MEASURE_REDUNDANT_BIND_CALLS
	measure_redundant_bind_call(id);
#endif
	CHECK_GL_ERROR(glBindTexture(GL_TEXTURE_2D, renderer.textures[id].id));
}

void load_texture(int id, const surface& surface, scale_option scaling, bool mipmap) {
	auto& texture = renderer.textures[id];
	texture.size = surface.dimensions();
	int format = gl_pixel_format(surface.format());
	bind_texture(id);
	CHECK_GL_ERROR(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texture.size.x, texture.size.y, 0, format, GL_UNSIGNED_BYTE, surface.data()));
	if (mipmap) {
		CHECK_GL_ERROR(glGenerateMipmap(GL_TEXTURE_2D));
	}
	CHECK_GL_ERROR(glTextureParameteri(texture.id, GL_TEXTURE_MAG_FILTER, gl_scale_option(scaling, false)));
	CHECK_GL_ERROR(glTextureParameteri(texture.id, GL_TEXTURE_MIN_FILTER, gl_scale_option(scaling, mipmap)));
	CHECK_GL_ERROR(glTextureParameteri(texture.id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
	CHECK_GL_ERROR(glTextureParameteri(texture.id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
}

void load_texture(int id, const surface& surface) {
	load_texture(id, surface, scale_option::nearest_neighbour, false);
}

void load_texture_from_screen(int id, int bottom_y, int x, int y, int width, int height) {
	auto& texture = renderer.textures[id];
	texture.size = { width, height };
	bind_texture(id);
	CHECK_GL_ERROR(glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, x, bottom_y - y - height, width, height, 0));
}

vector2i texture_size(int id) {
	return renderer.textures[id].size;
}

void delete_texture(int id) {
	if (id < 0) {
		return;
	}
	auto& texture = renderer.textures[id];
	CHECK_GL_ERROR(glDeleteTextures(1, &texture.id));
	texture = {};
}

static int create_shader_script(const std::string& source, unsigned int type) {
	const char* source_cstring = source.c_str();
	CHECK_GL_ERROR(int id = glCreateShader(type));
	CHECK_GL_ERROR(glShaderSource(id, 1, &source_cstring, 0));
	CHECK_GL_ERROR(glCompileShader(id));
#if DEBUG_ENABLED
	int length = 0;
	char buffer[1024];
	CHECK_GL_ERROR(glGetShaderInfoLog(id, 1024, &length, buffer));
	if (length > 0) {
		INFO("Shader info log: <br>" << buffer);
	}
#endif
	return id;
}

std::ostream& operator<<(std::ostream& out, const std::vector<std::string>& strings) {
	for (size_t i = 0; i < strings.size(); i++) {
		out << strings[i];
		if (strings.size() - 1 > i) {
			out << ", ";
		}
	}
	return out;
}

std::vector<std::string> find_vertex_shader_attributes(const std::string& source) {
	std::vector<std::string> attributes;
	size_t index = source.find("in ");
	while (index != std::string::npos) {
		// skip data type
		index += 3;
		while (source.size() > index) {
			index++;
			if (source[index] == ' ') {
				index++;
				break;
			}
		}
		// read name
		size_t end_index = source.find(';', index);
		if (end_index == std::string::npos) {
			break; // shader syntax error, so no need to log this
		}
		attributes.push_back(source.substr(index, end_index - index));
		index = source.find("in ", end_index);
	}
	return attributes;
}

int create_shader(const std::string& path) {
	int id = -1;
	for (int i = 0; i < (int)renderer.shaders.size(); i++) {
		if (renderer.shaders[i].id == 0) {
			id = i;
			break;
		}
	}
	if (id == -1) {
		renderer.shaders.emplace_back();
		id = (int)renderer.shaders.size() - 1;
	}
	auto& shader = renderer.shaders[id];
	CHECK_GL_ERROR(shader.id = glCreateProgram());

	std::string source = file::read(path + "/vertex.glsl");
	auto attributes = find_vertex_shader_attributes(source);
	MESSAGE("Loading shader " << path << " (" << attributes << ")");
	int vertex_shader_id = create_shader_script(source, GL_VERTEX_SHADER);
	CHECK_GL_ERROR(glAttachShader(shader.id, vertex_shader_id));
	for (size_t location = 0; location < attributes.size(); location++) {
		CHECK_GL_ERROR(glBindAttribLocation(shader.id, location, attributes[location].c_str()));
	}
	source = file::read(path + "/fragment.glsl");
	int fragment_shader_id = create_shader_script(source, GL_FRAGMENT_SHADER);
	CHECK_GL_ERROR(glAttachShader(shader.id, fragment_shader_id));
	CHECK_GL_ERROR(glLinkProgram(shader.id));
	CHECK_GL_ERROR(glDeleteShader(vertex_shader_id));
	CHECK_GL_ERROR(glDeleteShader(fragment_shader_id));

#if DEBUG_ENABLED
	char buffer[1024];
	int length = 0;
	CHECK_GL_ERROR(glGetProgramInfoLog(shader.id, 1024, &length, buffer));
	if (length > 0) {
		INFO("Shader program log " << shader.id << ": \n" << buffer);
	}

	CHECK_GL_ERROR(glValidateProgram(shader.id));

	int status = 0;
	CHECK_GL_ERROR(glGetProgramiv(shader.id, GL_VALIDATE_STATUS, &status));
	if (status == GL_FALSE) {
		CRITICAL("Failed to validate shader program with id " << shader.id);
	}
	ASSERT(status != GL_FALSE);
#endif

	bind_shader(id);

	CHECK_GL_ERROR(shader.model_view_projection_location = glGetUniformLocation(shader.id, "uni_ModelViewProjection"));
	CHECK_GL_ERROR(shader.model_location = glGetUniformLocation(shader.id, "uni_Model"));

	return id;
}

void bind_shader(int id) {
	if (renderer.bound_shader == id) {
		// todo: add to counter
		return;
	}
	renderer.bound_shader = id;
	CHECK_GL_ERROR(glUseProgram(renderer.shaders[id].id));
}

shader_variable get_shader_variable(const std::string& name) {
	return { renderer.shaders[renderer.bound_shader].id, name };
}

void set_shader_model(const transform& transform) {
	auto& shader = renderer.shaders[renderer.bound_shader];
	shader.model = transform_to_glm_mat4(transform);
	auto model_view_projection = shader.projection * shader.view * shader.model;
	CHECK_GL_ERROR(glUniformMatrix4fv(shader.model_view_projection_location, 1, false, glm::value_ptr(model_view_projection)));
	CHECK_GL_ERROR(glUniformMatrix4fv(shader.model_location, 1, false, glm::value_ptr(shader.model)));
}

void set_shader_view_projection(const ortho_camera& camera) {
	auto& shader = renderer.shaders[renderer.bound_shader];
	shader.view = camera.view();
	shader.projection = camera.projection();
	auto model_view_projection = shader.projection * shader.view * shader.model;
	CHECK_GL_ERROR(glUniformMatrix4fv(shader.model_view_projection_location, 1, false, glm::value_ptr(model_view_projection)));
}

void set_shader_view_projection(const perspective_camera& camera) {
	auto& shader = renderer.shaders[renderer.bound_shader];
	shader.view = camera.view();
	shader.projection = camera.projection();
	auto model_view_projection = shader.projection * shader.view * shader.model;
	CHECK_GL_ERROR(glUniformMatrix4fv(shader.model_view_projection_location, 1, false, glm::value_ptr(model_view_projection)));
}

void delete_shader(int id) {
	if (id < 0) {
		return;
	}
	auto& shader = renderer.shaders[id];
	CHECK_GL_ERROR(glDeleteProgram(shader.id));
	shader = {};
}

shader_variable::shader_variable(unsigned int program_id, const std::string& name) {
	CHECK_GL_ERROR(location = glGetUniformLocation(program_id, name.c_str()));
}

void shader_variable::set(int value) {
	CHECK_GL_ERROR(glUniform1i(location, value));
}

void shader_variable::set(float value) {
	CHECK_GL_ERROR(glUniform1f(location, value));
}

void shader_variable::set(const vector2f& vector) {
	CHECK_GL_ERROR(glUniform2fv(location, 1, &vector.x));
}

void shader_variable::set(const vector3f& vector) {
	CHECK_GL_ERROR(glUniform3fv(location, 1, &vector.x));
}

void shader_variable::set(const vector4f& vector) {
	CHECK_GL_ERROR(glUniform4fv(location, 1, &vector.x));
}

void shader_variable::set(const glm::mat4& matrix) {
	CHECK_GL_ERROR(glUniformMatrix4fv(location, 1, false, glm::value_ptr(matrix)));
}

void shader_variable::set(const transform& transform) {
	set(transform_to_glm_mat4(transform));
}

void shader_variable::set(vector2f* vector, size_t count) {
	CHECK_GL_ERROR(glUniform2fv(location, count, &vector[0].x));
}

void shader_variable::set(const std::vector<glm::mat4>& matrices) {
	CHECK_GL_ERROR(glUniformMatrix4fv(location, matrices.size(), GL_FALSE, glm::value_ptr(matrices[0])));
}

bool shader_variable::exists() const {
	return location != -1;
}

void set_polygon_render_mode(polygon_render_mode mode) {
	CHECK_GL_ERROR(glPolygonMode(GL_FRONT_AND_BACK, mode == polygon_render_mode::fill ? GL_FILL : GL_LINE));
}

vector3i read_pixel_at(vector2i position) {
	int alignment = 0;
	uint8_t pixel[3];
	CHECK_GL_ERROR(glFlush());
	CHECK_GL_ERROR(glFinish());
	CHECK_GL_ERROR(glGetIntegerv(GL_UNPACK_ALIGNMENT, &alignment));
	CHECK_GL_ERROR(glPixelStorei(GL_UNPACK_ALIGNMENT, 1));
	CHECK_GL_ERROR(glReadPixels(position.x, position.y, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, pixel));
	CHECK_GL_ERROR(glPixelStorei(GL_UNPACK_ALIGNMENT, alignment));
	return { pixel[0], pixel[1], pixel[2] };
}

}

#endif
