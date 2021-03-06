#version 330

uniform mat4 uni_ModelViewProjection;
uniform vec3 uni_Color;

in vec3 in_Position;
in vec3 in_Color;

flat out vec4 ex_Color;

void main() {
	gl_Position = uni_ModelViewProjection * vec4(in_Position, 1.0f);
	ex_Color = vec4(in_Color * uni_Color, 1.0f);
}
