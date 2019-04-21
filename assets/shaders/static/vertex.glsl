#version 330

uniform mat4 uni_ModelViewProjection;

in vec3 in_Position;
in vec2 in_TexCoords;

out vec2 ex_TexCoords;

void main() {
	gl_Position = uni_ModelViewProjection * vec4(in_Position, 1.0f);
	ex_TexCoords = in_TexCoords;
}
