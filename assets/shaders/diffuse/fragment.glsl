#version 330

uniform sampler2D uni_Texture;
uniform vec3 uni_LightPosition;
uniform vec3 uni_LightColor;

in vec4 ex_Color;
in vec2 ex_TexCoords;
in vec3 ex_Normal;
in vec3 ex_ModelPosition;

out vec4 out_Color;

void main() {
	float ambient = 0.25f;
	vec3 normal = normalize(ex_Normal);
	vec3 light = normalize(uni_LightPosition - ex_ModelPosition);
	float intensity = max(dot(normal, light), 0.0f);
	vec3 diffuse = (ambient + intensity) * uni_LightColor;
	out_Color = texture(uni_Texture, ex_TexCoords).rgba * ex_Color * vec4(diffuse, 1.0f);
}
