#version 130

uniform sampler2D uni_Texture;

in vec4 ex_Color;
in vec2 ex_TexCoords;

out vec4 out_Color;

void main() {
	out_Color = texture(uni_Texture, ex_TexCoords).rgba * ex_Color;
}
