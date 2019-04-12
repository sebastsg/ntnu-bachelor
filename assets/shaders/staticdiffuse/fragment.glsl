#version 330

uniform sampler2D uni_Texture;
uniform vec3 uni_LightPosition;
uniform vec3 uni_LightColor;
uniform vec4 uni_Color;

in vec2 ex_TexCoords;
in vec3 ex_Normal;
in vec3 ex_Position;

out vec4 out_Color;

float diffuse(float dist) {
	vec3 light = normalize(uni_LightPosition - ex_Position);
	float att = 1.0f / (1.0f + (0.001f * dist * dist));
	return max(dot(ex_Normal, light), 0.0f) * att;
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
	float intensity = min(1.0f, ambient + diffuse(dist));
	vec4 tex = texture(uni_Texture, ex_TexCoords).rgba;
	vec4 final = vec4(uni_LightColor * intensity, fog(dist));
	out_Color = tex * final * uni_Color;
}
