#include "vertex.hpp"
#include "draw.hpp"

#include "assimp/scene.h"
#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"

#include <filesystem>
#include <unordered_map>

namespace no {

static glm::mat4 ai_mat4_to_glm_mat4(const aiMatrix4x4& ai_matrix) {
	// note: assimp uses row-major order, while glm uses column-major order
	glm::mat4 glm_matrix;
	glm_matrix[0][0] = ai_matrix.a1;
	glm_matrix[1][0] = ai_matrix.a2;
	glm_matrix[2][0] = ai_matrix.a3;
	glm_matrix[3][0] = ai_matrix.a4;
	glm_matrix[0][1] = ai_matrix.b1;
	glm_matrix[1][1] = ai_matrix.b2;
	glm_matrix[2][1] = ai_matrix.b3;
	glm_matrix[3][1] = ai_matrix.b4;
	glm_matrix[0][2] = ai_matrix.c1;
	glm_matrix[1][2] = ai_matrix.c2;
	glm_matrix[2][2] = ai_matrix.c3;
	glm_matrix[3][2] = ai_matrix.c4;
	glm_matrix[0][3] = ai_matrix.d1;
	glm_matrix[1][3] = ai_matrix.d2;
	glm_matrix[2][3] = ai_matrix.d3;
	glm_matrix[3][3] = ai_matrix.d4;
	return glm_matrix;
}

static glm::quat ai_quat_to_glm_quat(const aiQuaternion& ai_quat) {
	return glm::quat(ai_quat.w, ai_quat.x, ai_quat.y, ai_quat.z);
}

template<typename V>
static vector3f find_min_vertex(const std::vector<V>& vertices) {
	vector3f min = FLT_MAX;
	for (auto& vertex : vertices) {
		min.x = std::min(min.x, vertex.position.x);
		min.y = std::min(min.y, vertex.position.y);
		min.z = std::min(min.z, vertex.position.z);
	}
	return min;
}

template<typename V>
static vector3f find_max_vertex(const std::vector<V>& vertices) {
	vector3f max = -FLT_MAX;
	for (auto& vertex : vertices) {
		max.x = std::max(max.x, vertex.position.x);
		max.y = std::max(max.y, vertex.position.y);
		max.z = std::max(max.z, vertex.position.z);
	}
	return max;
}

class assimp_importer {
public:

	model_data<animated_mesh_vertex> model;

	assimp_importer(const std::string& input, model_import_options options);

private:

	void load_animations();
	void load_mesh(aiMesh* mesh);
	void load_node(aiNode* node);

	model_import_options options;
	aiScene* scene = nullptr;
	int node_depth = 0;

};

assimp_importer::assimp_importer(const std::string& input, model_import_options options) : options(options) {
	int flags
		= aiProcess_Triangulate
		| aiProcess_ValidateDataStructure
		//| aiProcess_LimitBoneWeights maybe use this, and reduce bone limit from 6 to 4 at the same time?
		//| aiProcess_PreTransformVertices assimp doesn't like this flag here. todo: figure out why
		//| aiProcess_Debone look into this flag
		| aiProcess_FixInfacingNormals
		| aiProcess_GenSmoothNormals
		| aiProcess_CalcTangentSpace
		| aiProcess_FlipUVs
		| aiProcess_JoinIdenticalVertices
		| aiProcess_ImproveCacheLocality
		| aiProcess_OptimizeMeshes
		| aiProcess_OptimizeGraph;
	Assimp::Importer importer;
	scene = (aiScene*)importer.ReadFile(input, flags);
	if (!scene || !scene->mRootNode) {
		WARNING("No scene. Error loading model \"" << input << "\". Error: " << importer.GetErrorString());
		return;
	}
	if (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) {
		WARNING("Scene flags incomplete. Error loading model \"" << input << "\". Error: " << importer.GetErrorString());
	}
	model.name = std::filesystem::path(input).filename().stem().string();
	aiMatrix4x4 root_transform = scene->mRootNode->mTransformation;
	model.transform = ai_mat4_to_glm_mat4(root_transform.Inverse());
	load_node(scene->mRootNode);
	load_animations();
	model.min = find_min_vertex(model.shape.vertices);
	model.max = find_max_vertex(model.shape.vertices);
}

void assimp_importer::load_animations() {
	for (unsigned int a = 0; a < scene->mNumAnimations; a++) {
		auto& animation = model.animations.emplace_back();
		animation.name = scene->mAnimations[a]->mName.C_Str();
		if (animation.name == "") {
			animation.name = model.name;
			if (a > 0) {
				animation.name += std::to_string(a);
			}
		}
		animation.duration = (float)scene->mAnimations[a]->mDuration;
		animation.ticks_per_second = (float)scene->mAnimations[a]->mTicksPerSecond;
		animation.channels.insert(animation.channels.begin(), model.nodes.size(), {});
		for (unsigned int c = 0; c < scene->mAnimations[a]->mNumChannels; c++) {
			std::string animation_node_name = scene->mAnimations[a]->mChannels[c]->mNodeName.C_Str();
			model_animation_channel node;
			for (int bone_index = 0; bone_index < (int)model.bone_names.size(); bone_index++) {
				if (model.bone_names[bone_index] == animation_node_name) {
					node.bone = bone_index;
					break;
				}
			}
			auto ai_channel = scene->mAnimations[a]->mChannels[c];
			for (unsigned int p = 0; p < ai_channel->mNumPositionKeys; p++) {
				auto& key = ai_channel->mPositionKeys[p];
				node.positions.emplace_back((float)key.mTime, vector3f{ key.mValue.x, key.mValue.y, key.mValue.z });
			}
			for (unsigned int r = 0; r < ai_channel->mNumRotationKeys; r++) {
				auto& key = ai_channel->mRotationKeys[r];
				node.rotations.emplace_back((float)key.mTime, ai_quat_to_glm_quat(key.mValue));
			}
			for (unsigned int s = 0; s < ai_channel->mNumScalingKeys; s++) {
				auto& key = ai_channel->mScalingKeys[s];
				node.scales.emplace_back((float)key.mTime, vector3f{ key.mValue.x, key.mValue.y, key.mValue.z });
			}
			bool found_node = false;
			for (size_t n = 0; n < model.nodes.size(); n++) {
				if (model.nodes[n].name == animation_node_name) {
					/*INFO("[b]Channel #" << c << "[/b]: " << animation_node_name << " uses bone " << node.bone << " as node " << n
						 << " (positions = " << node.positions.size()
						 << ", rotations = " << node.rotations.size()
						 << ", scale = " << node.scales.size() << ")");*/
					animation.channels[n] = node;
					found_node = true;
					break;
				}
			}
			if (!found_node) {
				WARNING("[b]Channel #" << c << "[/b] " << animation_node_name << " has no matching node. Ignoring");
			}
		}
	}
}

void assimp_importer::load_node(aiNode* node) {
	/*MESSAGE(std::string(node_depth, '.') << model.nodes.size() << " [b]" << node->mName.C_Str() << "[/b]: "
			<< node->mNumMeshes << " meshes / " << node->mNumChildren << " children");*/
	int index = (int)model.nodes.size();
	model.nodes.emplace_back();
	model.nodes[index].transform = ai_mat4_to_glm_mat4(node->mTransformation);
	model.nodes[index].name = node->mName.C_Str();
	model.nodes[index].depth = node_depth;
	for (unsigned int i = 0; i < node->mNumMeshes; i++) {
		load_mesh(scene->mMeshes[node->mMeshes[i]]);
	}
	node_depth++;
	for (unsigned int i = 0; i < node->mNumChildren; i++) {
		model.nodes[index].children.push_back((int)model.nodes.size());
		load_node(node->mChildren[i]);
	}
	node_depth--;
}

void assimp_importer::load_mesh(aiMesh* mesh) {
	aiColor4D material_color;
	aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
	aiGetMaterialColor(material, AI_MATKEY_COLOR_DIFFUSE, &material_color);
	model.shape.vertices.reserve(mesh->mNumVertices);
	for (unsigned int v = 0; v < mesh->mNumVertices; v++) {
		animated_mesh_vertex vertex;
		if (mesh->HasPositions()) {
			vertex.position.x = mesh->mVertices[v].x;
			vertex.position.y = mesh->mVertices[v].y;
			vertex.position.z = mesh->mVertices[v].z;
		}
		if (mesh->HasVertexColors(0)) {
			vertex.color.x = mesh->mColors[0][v].r;
			vertex.color.y = mesh->mColors[0][v].g;
			vertex.color.z = mesh->mColors[0][v].b;
		} else {
			vertex.color.x = material_color.r;
			vertex.color.y = material_color.g;
			vertex.color.z = material_color.b;
		}
		if (mesh->HasTextureCoords(0)) {
			vertex.tex_coords.x = mesh->mTextureCoords[0][v].x;
			vertex.tex_coords.y = mesh->mTextureCoords[0][v].y;
		}
		if (mesh->HasNormals()) {
			vertex.normal.x = mesh->mNormals[v].x;
			vertex.normal.y = mesh->mNormals[v].y;
			vertex.normal.z = mesh->mNormals[v].z;
		}
		if (mesh->HasTangentsAndBitangents()) {
			vertex.tangent.x = mesh->mTangents[v].x;
			vertex.tangent.y = mesh->mTangents[v].y;
			vertex.tangent.z = mesh->mTangents[v].z;
			vertex.bitangent.x = mesh->mBitangents[v].x;
			vertex.bitangent.y = mesh->mBitangents[v].y;
			vertex.bitangent.z = mesh->mBitangents[v].z;
		}
		model.shape.vertices.emplace_back(vertex);
	}
	for (int b = 0; b < (int)mesh->mNumBones; b++) {
		std::string bone_name = mesh->mBones[b]->mName.C_Str();
		//MESSAGE("Bone #" << b << " " << bone_name);
		model.bones.emplace_back(ai_mat4_to_glm_mat4(mesh->mBones[b]->mOffsetMatrix));
		model.bone_names.push_back(bone_name);
		for (unsigned int w = 0; w < mesh->mBones[b]->mNumWeights; w++) {
			auto& weight = mesh->mBones[b]->mWeights[w];
			if (weight.mVertexId >= model.shape.vertices.size()) {
				WARNING("Vertex " << weight.mVertexId << " does not exist. Cannot add weight " << weight.mWeight);
				continue;
			}
			auto& vertex = model.shape.vertices[weight.mVertexId];
			// todo: maybe refactor this - also, the second half of this should maybe do more comparisons of weights
			if (vertex.weights.x < 0.0001f) {
				vertex.weights.x = weight.mWeight;
				vertex.bones.x = b;
			} else if (vertex.weights.y < 0.0001f) {
				vertex.weights.y = weight.mWeight;
				vertex.bones.y = b;
			} else if (vertex.weights.z < 0.0001f) {
				vertex.weights.z = weight.mWeight;
				vertex.bones.z = b;
			} else if (vertex.weights.w < 0.0001f) {
				vertex.weights.w = weight.mWeight;
				vertex.bones.w = b;
			} else if (vertex.weights_extra.x < 0.0001f) {
				vertex.weights_extra.x = weight.mWeight;
				vertex.bones_extra.x = b;
			} else if (vertex.weights_extra.y < 0.0001f) {
				vertex.weights_extra.y = weight.mWeight;
				vertex.bones_extra.y = b;
			} else if (weight.mWeight > vertex.weights.x) {
				WARNING("Discarding weight #0 = " << vertex.weights.x << " for vertex " << weight.mVertexId);
				vertex.weights.x = weight.mWeight;
				vertex.bones.x = b;
			} else if (weight.mWeight > vertex.weights.y) {
				WARNING("Discarding weight #1 = " << vertex.weights.y << " for vertex " << weight.mVertexId);
				vertex.weights.y = weight.mWeight;
				vertex.bones.y = b;
			} else if (weight.mWeight > vertex.weights.z) {
				WARNING("Discarding weight #2 = " << vertex.weights.z << " for vertex " << weight.mVertexId);
				vertex.weights.z = weight.mWeight;
				vertex.bones.z = b;
			} else if (weight.mWeight > vertex.weights.w) {
				WARNING("Discarding weight #3 = " << vertex.weights.w << " for vertex " << weight.mVertexId);
				vertex.weights.w = weight.mWeight;
				vertex.bones.w = b;
			} else if (weight.mWeight > vertex.weights_extra.x) {
				WARNING("Discarding weight #4 = " << vertex.weights_extra.x<< " for vertex " << weight.mVertexId);
				vertex.weights_extra.x = weight.mWeight;
				vertex.bones_extra.x = b;
			} else if (weight.mWeight > vertex.weights_extra.y) {
				WARNING("Discarding weight #5 = " << vertex.weights_extra.y << " for vertex " << weight.mVertexId);
				vertex.weights_extra.y = weight.mWeight;
				vertex.bones_extra.y = b;
			} else {
				WARNING("Discarding weight #6 = " << weight.mWeight << " for vertex " << weight.mVertexId);
			}
		}
	}
	if (mesh->mNumBones == 0 && options.bones.create_default) {
		//INFO("No bones. Creating default bone: " << options.bones.bone_name);
		model.bones.emplace_back(glm::mat4(1.0f));
		for (auto& vertex : model.shape.vertices) {
			vertex.weights.x = 1.0f;
			vertex.bones.x = 0;
		}
		model.bone_names.push_back(options.bones.bone_name);
	}
	model.shape.indices.reserve(mesh->mNumFaces * 3);
	for (unsigned int f = 0; f < mesh->mNumFaces; f++) {
		for (unsigned int i = 0; i < mesh->mFaces[f].mNumIndices; i++) {
			model.shape.indices.push_back(mesh->mFaces[f].mIndices[i]);
		}
	}
}

void convert_model(const std::string& source, const std::string& destination, model_conversion_options options) {
	if (!options.exporter) {
		return;
	}
	assimp_importer importer(source, options.import);
	options.exporter(destination, importer.model);
}

transform3 load_model_bounding_box(const std::string& path) {
	io_stream stream;
	file::read(path, stream);
	if (stream.write_index() == 0) {
		WARNING("Failed to open file: " << path);
		return {};
	}
	stream.move_read_index(sizeof(glm::mat4));
	transform3 transform;
	transform.position = stream.read<vector3f>();
	transform.scale = stream.read<vector3f>();
	return transform;
}

}
