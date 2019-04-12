#version 330

uniform mat4 uni_ModelViewProjection;
uniform mat4 uni_Model;

in vec3 in_Position;
in vec3 in_Normal;
in vec2 in_TexCoords;

out vec2 ex_TexCoords;
out vec3 ex_Normal;
out vec3 ex_Position;

void main() {
	gl_Position = uni_ModelViewProjection * vec4(in_Position, 1.0f);
	ex_TexCoords = in_TexCoords;
	ex_Normal = vec3(uni_Model * vec4(in_Normal, 0.0f));
	ex_Position = vec3(uni_Model * vec4(in_Position, 1.0f));
}
