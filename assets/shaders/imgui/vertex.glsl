#version 330

uniform mat4 uni_ModelViewProjection;

in vec2 in_Position;
in vec2 in_TexCoords;
in vec4 in_Color;

out vec4 ex_Color;
out vec2 ex_TexCoords;

void main() {
	gl_Position = uni_ModelViewProjection * vec4(in_Position.xy, 0.0f, 1.0f);
	ex_Color = in_Color;
	ex_TexCoords = in_TexCoords;
}
