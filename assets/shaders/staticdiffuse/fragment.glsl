#version 330

uniform sampler2D uni_Texture;
uniform vec3 uni_LightPosition;
uniform vec3 uni_LightColor;
uniform float uni_FogStart;
uniform float uni_FogDistance;
uniform vec4 uni_Color;

in vec2 ex_TexCoords;
in vec3 ex_Normal;
in vec3 ex_Position;

out vec4 out_Color;

void main() {
	float ambient = 0.75f;
	float distance = length(uni_LightPosition - ex_Position);
	vec3 light = normalize(uni_LightPosition - ex_Position);
	float diffuse = max(dot(ex_Normal, light), 0.0f);
	diffuse = diffuse * (1.0f / (1.0f + (0.1f * distance * distance)));
	vec3 result = min(1.0f, ambient + diffuse) * uni_LightColor;
	float fog = distance > uni_FogStart ? (distance - uni_FogStart) / uni_FogDistance : 0.0f;
	out_Color = texture(uni_Texture, ex_TexCoords).rgba * vec4(result, min(1.0f, 1.0f - fog)) * uni_Color;
}
