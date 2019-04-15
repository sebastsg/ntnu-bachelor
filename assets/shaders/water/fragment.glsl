#version 330

uniform sampler2D uni_Texture;
uniform vec3 uni_LightPosition;
uniform vec3 uni_LightColor;

in vec3 ex_Position;

out vec4 out_Color;

float directional(float dist) {
	vec3 ldir = vec3(0.3f, -1.0f, -0.3f);
	vec3 dir = normalize(-ldir);
	float att = 1.0f / (1.0f + (0.001f * dist * dist));
	return max(dot(vec3(0.0f, 1.0f, 0.0f), dir), 0.0f) * att;
}

float diffuse(float dist) {
	vec3 light = normalize(uni_LightPosition - ex_Position);
	float att = 1.0f / (1.0f + (0.001f * dist * dist));
	return max(dot(vec3(0.0f, -1.0f, 0.0f), light), 0.0f) * att;
}

float fog(float dist) {
	float start = 50.0f;
	float end = 20.0f;
	if (start > dist) {
		return 1.0f;
	}
	float f = (dist - start) / end;
	return 1.0f - max(f, 0.0f);
}

void main() {
	float ambient = 0.6f;
	float dist = length(uni_LightPosition - ex_Position);
	float intensity = min(1.0f, ambient + directional(dist));
	out_Color = vec4(vec3(0.2f, 0.6f, 1.0f) * intensity, min(0.4f, fog(dist)));
}
