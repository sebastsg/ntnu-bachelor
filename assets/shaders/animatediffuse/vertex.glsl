#version 330

uniform mat4 uni_ModelViewProjection;
uniform mat4 uni_Model;
uniform mat4 uni_Bones[100];

in vec3 in_Position;
in vec4 in_Color;
in vec2 in_TexCoords;
in vec3 in_Normal;
in vec3 in_Tangent;
in vec3 in_Bitangent;
in vec4 in_Weights;
in vec2 in_WeightsExtra;
in ivec4 in_Bones;
in ivec2 in_BonesExtra;

out vec2 ex_TexCoords;
out vec3 ex_Normal;
out vec3 ex_Position;

void main() {
	mat4 bone = uni_Bones[in_Bones.x] * in_Weights.x;
	bone += uni_Bones[in_Bones.y] * in_Weights.y;
	bone += uni_Bones[in_Bones.z] * in_Weights.z;
	bone += uni_Bones[in_Bones.w] * in_Weights.w;
	bone += uni_Bones[in_BonesExtra.x] * in_WeightsExtra.x;
	bone += uni_Bones[in_BonesExtra.y] * in_WeightsExtra.y;
	vec4 animatedPosition = bone * vec4(in_Position, 1.0f);
	vec4 animatedNormal = bone * vec4(in_Normal, 0.0f);
	gl_Position = uni_ModelViewProjection * animatedPosition;
	ex_TexCoords = in_TexCoords;
	ex_Normal = vec3(uni_Model * animatedNormal);
	ex_Position = vec3(uni_Model * animatedPosition);
}
