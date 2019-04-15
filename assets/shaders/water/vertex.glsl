#version 330

uniform mat4 uni_ModelViewProjection;
uniform mat4 uni_Model;

in vec3 in_Position;

out vec3 ex_Position;

void main() {
	gl_Position = uni_ModelViewProjection * vec4(in_Position, 1.0f);
	ex_Position = vec3(uni_Model * vec4(in_Position, 1.0f));
}
