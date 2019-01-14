#version 330

uniform mat4 uni_ModelViewProjection;

in vec3 in_Position;
in vec3 in_Color;
in vec2 in_TexCoords;

out vec4 ex_Color;
out vec2 ex_TexCoords;

void main() {
	gl_Position = uni_ModelViewProjection * vec4(in_Position, 1.0f);
	ex_Color = vec4(in_Color, 1.0f);
	ex_TexCoords = in_TexCoords;
}
