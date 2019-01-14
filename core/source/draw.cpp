#include "draw.hpp"
#include "debug.hpp"
#include "assets.hpp"
#include "io.hpp"

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

class assimp_to_nom_exporter {
public:

	assimp_to_nom_exporter(const std::string& input, const std::string& output, model_importer_options options);

private:

	aiScene* scene = nullptr;
	model_data model;
	int node_depth = 0;
	std::vector<std::string> bone_names;
	std::string file_name;
	model_importer_options options;

	void load_animations();
	void load_mesh(aiMesh* mesh);
	void load_node(aiNode* node);

};

assimp_to_nom_exporter::assimp_to_nom_exporter(const std::string& input, const std::string& output, model_importer_options options) : options(options) {
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
	file_name = std::filesystem::path(input).filename().stem().string();
	aiMatrix4x4 root_transform = scene->mRootNode->mTransformation;
	model.transform = ai_mat4_to_glm_mat4(root_transform.Inverse());
	load_node(scene->mRootNode);
	INFO("Loaded " << model.nodes.size() << " nodes");
	load_animations();
	export_model(output, model);
}

void assimp_to_nom_exporter::load_animations() {
	INFO("This model has " << scene->mNumAnimations << " animations");
	for (unsigned int a = 0; a < scene->mNumAnimations; a++) {
		auto& animation = model.animations.emplace_back();
		animation.name = scene->mAnimations[a]->mName.C_Str();
		if (animation.name == "") {
			animation.name = file_name;
			if (a > 0) {
				animation.name += std::to_string(a);
			}
			INFO("Animation #" << a << " named \"" << animation.name << "\" to avoid empty name");
		}
		animation.duration = (float)scene->mAnimations[a]->mDuration;
		animation.ticks_per_second = (float)scene->mAnimations[a]->mTicksPerSecond;
		animation.channels.insert(animation.channels.begin(), model.nodes.size(), {});
		for (unsigned int c = 0; c < scene->mAnimations[a]->mNumChannels; c++) {
			std::string animation_node_name = scene->mAnimations[a]->mChannels[c]->mNodeName.C_Str();
			model_animation_channel node;
			for (int bone_index = 0; bone_index < (int)bone_names.size(); bone_index++) {
				if (bone_names[bone_index] == animation_node_name) {
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
					INFO("[b]Channel #" << c << "[/b]: " << animation_node_name << " uses bone " << node.bone << " as node " << n 
						 << " (positions = " << node.positions.size() 
						 << ", rotations = " << node.rotations.size() 
						 << ", scale = " << node.scales.size() << ")");
					animation.channels[n] = node;
					found_node = true;
					break;
				}
			}
			if (!found_node) {
				WARNING("[b]Channel #" << c << "[/b] " << animation_node_name << " has no matching node. It will be ignored.");
			}
		}
	}
}

void assimp_to_nom_exporter::load_node(aiNode* node) {
#if DEBUG_ENABLED
	std::string depth;
	for (int i = 0; i < node_depth; i++) {
		depth += " . ";
	}
	MESSAGE(depth << model.nodes.size() << " [b]" << node->mName.C_Str() << "[/b]: "
			<< node->mNumMeshes << " meshes / " << node->mNumChildren << " children");
#endif
	node_depth++;
	int index = (int)model.nodes.size();
	model.nodes.emplace_back();
	model.nodes[index].transform = ai_mat4_to_glm_mat4(node->mTransformation);
	model.nodes[index].name = node->mName.C_Str();
	for (unsigned int i = 0; i < node->mNumMeshes; i++) {
		load_mesh(scene->mMeshes[node->mMeshes[i]]);
	}
	for (unsigned int i = 0; i < node->mNumChildren; i++) {
		model.nodes[index].children.push_back((int)model.nodes.size());
		load_node(node->mChildren[i]);
	}
	node_depth--;
}

void assimp_to_nom_exporter::load_mesh(aiMesh* mesh) {
	aiColor4D material_color;
	aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
	aiGetMaterialColor(material, AI_MATKEY_COLOR_DIFFUSE, &material_color);
	INFO("Loading " << mesh->mNumVertices << " vertices");
	model.shape.vertices.reserve(mesh->mNumVertices);
	for (unsigned int v = 0; v < mesh->mNumVertices; v++) {
		mesh_vertex vertex;
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

	INFO("Loading " << mesh->mNumBones << " bones");
	for (int b = 0; b < (int)mesh->mNumBones; b++) {
		std::string bone_name = mesh->mBones[b]->mName.C_Str();
		MESSAGE("Bone #" << b << " " << bone_name);
		model.bones.emplace_back(ai_mat4_to_glm_mat4(mesh->mBones[b]->mOffsetMatrix));
		bone_names.push_back(bone_name);
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
		INFO("No bones. Creating default bone: " << options.bones.bone_name);
		model.bones.emplace_back(glm::mat4(1.0f));
		for (auto& vertex : model.shape.vertices) {
			vertex.weights.x = 1.0f;
			vertex.bones.x = 0;
		}
		bone_names.push_back(options.bones.bone_name);
	}

	model.shape.indices.reserve(mesh->mNumFaces * 3);
	for (unsigned int f = 0; f < mesh->mNumFaces; f++) {
		for (unsigned int i = 0; i < mesh->mFaces[f].mNumIndices; i++) {
			model.shape.indices.push_back(mesh->mFaces[f].mIndices[i]);
		}
	}
	INFO("Loaded " << model.shape.indices.size() << " indices");
}

void export_model(const std::string& path, model_data& model) {
	io_stream stream;
	stream.write(model.transform);
	stream.write((int32_t)model.shape.vertices.size());
	for (auto& vertex : model.shape.vertices) {
		stream.write(vertex.position);
		stream.write(vertex.color);
		stream.write(vertex.tex_coords);
		stream.write(vertex.normal);
		stream.write(vertex.tangent);
		stream.write(vertex.bitangent);
		stream.write(vertex.weights);
		stream.write(vertex.weights_extra);
		stream.write((uint16_t)vertex.bones.x);
		stream.write((uint16_t)vertex.bones.y);
		stream.write((uint16_t)vertex.bones.z);
		stream.write((uint16_t)vertex.bones.w);
		stream.write((uint16_t)vertex.bones_extra.x);
		stream.write((uint16_t)vertex.bones_extra.y);
	}
	stream.write((int32_t)model.shape.indices.size());
	for (auto& index : model.shape.indices) {
		stream.write((uint16_t)index);
	}
	stream.write((int16_t)model.bones.size());
	for (auto& bone : model.bones) {
		stream.write(bone);
	}
	stream.write((int16_t)model.nodes.size());
	for (auto& node : model.nodes) {
		stream.write(node.name);
		stream.write(node.transform);
		stream.write((int16_t)node.children.size());
		for (auto& child : node.children) {
			stream.write((int16_t)child);
		}
	}
	stream.write((int16_t)model.animations.size());
	for (auto& animation : model.animations) {
		stream.write(animation.name);
		stream.write(animation.duration);
		stream.write(animation.ticks_per_second);
		stream.write((int16_t)animation.channels.size());
		for (auto& node : animation.channels) {
			stream.write((int16_t)node.bone);
			stream.write((int16_t)node.positions.size());
			for (auto& position : node.positions) {
				stream.write(position.first);
				stream.write(position.second);
			}
			stream.write((int16_t)node.rotations.size());
			for (auto& rotation : node.rotations) {
				stream.write(rotation.first);
				stream.write(rotation.second);
			}
			stream.write((int16_t)node.scales.size());
			for (auto& scale : node.scales) {
				stream.write(scale.first);
				stream.write(scale.second);
			}
		}
		stream.write((int16_t)animation.transitions.size());
		for (auto& transition : animation.transitions) {
			stream.write((int16_t)transition);
		}
	}
	file::write(path, stream);
}

void import_model(const std::string& path, model_data& model) {
	io_stream stream(10000);
	file::read(path, stream);
	if (stream.write_index() == 0) {
		WARNING("Failed to open model file: " << path);
		return;
	}
	model.transform = stream.read<glm::mat4>();
	int32_t vertex_count = stream.read<int32_t>();
	for (int32_t v = 0; v < vertex_count; v++) {
		mesh_vertex vertex;
		vertex.position = stream.read<vector3f>();
		vertex.color = stream.read<vector4f>();
		vertex.tex_coords = stream.read<vector2f>();
		vertex.normal = stream.read<vector3f>();
		vertex.tangent = stream.read<vector3f>();
		vertex.bitangent = stream.read<vector3f>();
		vertex.weights = stream.read<vector4f>();
		vertex.weights_extra = stream.read<vector2f>();
		vertex.bones.x = (int)stream.read<uint16_t>();
		vertex.bones.y = (int)stream.read<uint16_t>();
		vertex.bones.z = (int)stream.read<uint16_t>();
		vertex.bones.w = (int)stream.read<uint16_t>();
		vertex.bones_extra.x = (int)stream.read<uint16_t>();
		vertex.bones_extra.y = (int)stream.read<uint16_t>();
		model.shape.vertices.push_back(vertex);
	}
	int32_t index_count = stream.read<int32_t>();
	for (int32_t i = 0; i < index_count; i++) {
		model.shape.indices.push_back(stream.read<uint16_t>());
	}
	int16_t bone_count = stream.read<int16_t>();
	for (int16_t b = 0; b < bone_count; b++) {
		model.bones.emplace_back(stream.read<glm::mat4>());
	}
	int16_t node_count = stream.read<int16_t>();
	for (int16_t n = 0; n < node_count; n++) {
		auto& node = model.nodes.emplace_back();
		node.name = stream.read<std::string>();
		node.transform = stream.read<glm::mat4>();
		int16_t child_count = stream.read<int16_t>();
		for (int16_t c = 0; c < child_count; c++) {
			node.children.push_back(stream.read<int16_t>());
		}
	}
	int16_t animation_count = stream.read<int16_t>();
	for (int16_t a = 0; a < animation_count; a++) {
		auto& animation = model.animations.emplace_back();
		animation.name = stream.read<std::string>();
		animation.duration = stream.read<float>();
		animation.ticks_per_second = stream.read<float>();
		int16_t node_count = stream.read<int16_t>();
		for (int16_t n = 0; n < node_count; n++) {
			auto& node = animation.channels.emplace_back();
			node.bone = (int)stream.read<int16_t>();
			int16_t position_count = stream.read<int16_t>();
			for (int16_t p = 0; p < position_count; p++) {
				auto& position = node.positions.emplace_back();
				position.first = stream.read<float>();
				position.second = stream.read<vector3f>();
			}
			int16_t rotation_count = stream.read<int16_t>();
			for (int16_t r = 0; r < rotation_count; r++) {
				auto& rotation = node.rotations.emplace_back();
				rotation.first = stream.read<float>();
				rotation.second = stream.read<glm::quat>();
			}
			int16_t scale_count = stream.read<int16_t>();
			for (int16_t s = 0; s < scale_count; s++) {
				auto& scale = node.scales.emplace_back();
				scale.first = stream.read<float>();
				scale.second = stream.read<vector3f>();
			}
		}
		int16_t transition_count = stream.read<int16_t>();
		for (int16_t t = 0; t < transition_count; t++) {
			animation.transitions.push_back((int)stream.read<int16_t>());
		}
	}
}

void convert_model(const std::string& source, const std::string& destination, model_importer_options options) {
	assimp_to_nom_exporter exporter(source, destination, options);
}

void convert_model(const std::string& source, const std::string& destination) {
	assimp_to_nom_exporter exporter(source, destination, {});
}

void merge_model_animations(const std::string& source_directory, const std::string& destination) {
	std::vector<std::string> loaded_paths; // can't use "paths", since some files may be invalid
	std::vector<model_data> models;
	auto paths = entries_in_directory(source_directory, entry_inclusion::only_files);
	for (auto& path : paths) {
		auto extension = file_extension_in_path(path);
		if (extension == ".txt") {
			continue; // transitions.txt is used later
		}
		if (extension != ".nom") {
			WARNING("Skipping " << path << " as it is not a .nom file");
			continue;
		}
		import_model(path, models.emplace_back());
		loaded_paths.push_back(path);
	}
	if (models.empty()) {
		WARNING("No models were found in " << source_directory);
		return;
	}
	if (models.size() == 1) {
		WARNING("Only one model was found. It will be copied to its destination, but no merging is done.");
	}
	INFO("Using " << loaded_paths.front() << " as reference and output model");
	auto& output = models.front();
	for (size_t m = 1; m < models.size(); m++) {
		auto& model_path = loaded_paths[m];
		auto& model = models[m];
		if (model.transform != output.transform) {
			WARNING("Root transform was not identical for " << model_path << ". Note: Will not be skipped");
		}
		if (output.nodes.size() != model.nodes.size()) {
			WARNING(model_path << " has different number of node transforms. Skipping.");
			continue;
		}
		bool equal = true;
		for (size_t n = 0; n < output.nodes.size(); n++) {
			if (output.nodes[n].transform != model.nodes[n].transform) {
				WARNING(model_path << " has a different node transform at index " << n << ". Skipping.");
				equal = false;
				break;
			}
			if (output.nodes[n].children != model.nodes[n].children) {
				WARNING(model_path << " has different node children at index " << n << ". Skipping.");
				equal = false;
				break;
			}
		}
		if (!equal) {
			continue;
		}
		if (model.bones.size() != output.bones.size()) {
			WARNING("Mesh was not identical in " << model_path << ". Skipping.");
			continue;
		}
		for (size_t i = 0; i < model.bones.size(); i++) {
			if (model.bones[i] != output.bones[i]) {
				WARNING("Mesh was not identical in " << model_path << ". Skipping.");
				continue;
			}
		}
		if (model.shape != output.shape) {
			WARNING("Mesh was not identical in " << model_path << ". Skipping.");
			continue;
		}
		for (auto& animation : model.animations) {
			bool skip = false;
			for (auto& existing_animation : output.animations) {
				if (animation.name == existing_animation.name) {
					WARNING("Animation " << animation.name << " in " << model_path << " already exists. Skipping this animation.");
					skip = true;
					continue;
				}
			}
			if (skip) {
				continue;
			}
			output.animations.push_back(animation);
		}
	}
	std::vector<int> default_transitions;
	default_transitions.insert(default_transitions.begin(), output.animations.size(), 0);
	for (auto& animation : output.animations) {
		animation.transitions = default_transitions;
	}

	io_stream transitions_stream;
	std::string transitions_path = source_directory + "/transitions.txt";
	file::read(transitions_path, transitions_stream);
	int current_line = 0;
	while (true) {
		std::string line = transitions_stream.read_line(true);
		if (line.empty()) {
			break;
		}
		current_line++;

		// todo: better off using regex here

		size_t first_space = line.find(' ');
		if (first_space == std::string::npos) {
			WARNING(transitions_path << " error on line " << current_line);
			continue;
		}
		std::string from = line.substr(0, first_space);
		size_t arrow_begin = line.find("->", first_space);
		if (arrow_begin == std::string::npos) {
			WARNING(transitions_path << " error on line " << current_line << ". -> was not found.");
			continue;
		}
		size_t arrow_end = arrow_begin + 2;
		size_t equals_sign = line.find('=', arrow_end);
		if (equals_sign == std::string::npos) {
			WARNING(transitions_path << " error on line " << current_line << ". = was not found.");
			continue;
		}
		std::string to = line.substr(arrow_end + 1, equals_sign - 1 - arrow_end - 1);
		std::string frame = line.substr(equals_sign + 1);
		if (from == to) {
			WARNING(transitions_path << " error on line " << current_line << ". " << from << " -> " << to << " are identical.");
			continue;
		}
		bool found_from = false;
		for (auto& animation : output.animations) {
			if (animation.name == from) {
				bool found_to = false;
				for (int to_index = 0; to_index < (int)output.animations.size(); to_index++) {
					if (output.animations[to_index].name == to) {
						animation.transitions[to_index] = std::stoi(frame);
						found_to = true;
						break;
					}
				}
				if (!found_to) {
					WARNING(transitions_path << " error for " << from << " -> " << to << ". " << to << " was not found.");
				}
				found_from = true;
				break;
			}
		}
		if (!found_from) {
			WARNING(transitions_path << " error for " << from << " -> " << to << ". " << from << " was not found.");
		}
	}

	if (file_extension_in_path(destination) != ".nom") {
		WARNING("Output file " << destination << " does not end with the .nom extension");
	}
	export_model(destination, output);
}

model::model(model&& that) : mesh(std::move(that.mesh)) {
	std::swap(root_transform, that.root_transform);
	std::swap(bones, that.bones);
	std::swap(nodes, that.nodes);
	std::swap(animations, that.animations);
}

model::model(const std::string& path) {
	load(path);
}

model& model::operator=(model&& that) {
	std::swap(mesh, that.mesh);
	std::swap(root_transform, that.root_transform);
	std::swap(bones, that.bones);
	std::swap(nodes, that.nodes);
	std::swap(animations, that.animations);
	return *this;
}

int model::index_of_animation(const std::string& name) {
	for (int i = 0; i < (int)animations.size(); i++) {
		if (animations[i].name == name) {
			return i;
		}
	}
	return -1;
}

int model::total_animations() const {
	return (int)animations.size();
}

void model::load(const std::string& path) {
	model_data model;
	import_model(path, model);
	if (model.shape.vertices.empty()) {
		WARNING("Failed to load model: " << path);
	}
	root_transform = model.transform;
	nodes = model.nodes;
	mesh = { model.shape.vertices, model.shape.indices };
	bones = model.bones;
	animations = model.animations;
	size_t vertices = model.shape.vertices.size();
	size_t indices = model.shape.indices.size();
	MESSAGE("Loaded mesh with " << vertices << " vertices and " << indices << " indices. Model: " << path);
}

void model::draw() const {
	mesh.bind();
	mesh.draw();
}

model_instance::model_instance(model& source) : source(source), bones(source.bones) {
	animation_timer.start();
	for (auto& animation : source.animations) {
		animations.emplace_back();
		for (auto& node : animation.channels) {
			animations.back().channels.emplace_back();
		}
	}
}

model_instance::model_instance(model_instance&& that) : source(std::move(that.source)) {
	std::swap(bones, that.bones);
	std::swap(animations, that.animations);
	std::swap(attachments, that.attachments);
	std::swap(attachment_id_counter, that.attachment_id_counter);
	std::swap(is_new_loop, that.is_new_loop);
	std::swap(is_new_animation, that.is_new_animation);
	std::swap(new_animation_transition_frame, that.new_animation_transition_frame);
	std::swap(animation_index, that.animation_index);
	std::swap(animation_timer, that.animation_timer);
}

model_instance& model_instance::operator=(model_instance&& that) {
	std::swap(source, that.source);
	std::swap(bones, that.bones);
	std::swap(animations, that.animations);
	std::swap(attachments, that.attachments);
	std::swap(attachment_id_counter, that.attachment_id_counter);
	std::swap(is_new_loop, that.is_new_loop);
	std::swap(is_new_animation, that.is_new_animation);
	std::swap(new_animation_transition_frame, that.new_animation_transition_frame);
	std::swap(animation_index, that.animation_index);
	std::swap(animation_timer, that.animation_timer);
	return *this;
}

model& model_instance::original() const {
	return source;
}

glm::mat4 model_instance::next_interpolated_position(int node_index, float time) {
	auto& node = source.animations[animation_index].channels[node_index];
	for (int p = animations[animation_index].channels[node_index].last_position_key; p < (int)node.positions.size() - 1; p++) {
		if (node.positions[p + 1].first > time) {
			animations[animation_index].channels[node_index].last_position_key = p;
			auto& current = node.positions[p];
			auto& next = node.positions[p + 1];
			float delta_time = next.first - current.first;
			float factor = (time - current.first) / delta_time;
			ASSERT(factor >= 0.0f && factor <= 1.0f);
			vector3f delta_position = next.second - current.second;
			vector3f interpolated_delta = factor * delta_position;
			vector3f translation = current.second + interpolated_delta;
			return glm::translate(glm::mat4(1.0f), { translation.x, translation.y, translation.z });
		}
	}
	if (node.positions.empty()) {
		return glm::mat4(1.0f);
	}
	auto& translation = node.positions.front().second;
	return glm::translate(glm::mat4(1.0f), { translation.x, translation.y, translation.z });
}

glm::mat4 model_instance::next_interpolated_rotation(int node_index, float time) {
	auto& node = source.animations[animation_index].channels[node_index];
	for (int r = animations[animation_index].channels[node_index].last_rotation_key; r < (int)node.rotations.size() - 1; r++) {
		if (node.rotations[r + 1].first > time) {
			animations[animation_index].channels[node_index].last_rotation_key = r;
			auto& current = node.rotations[r];
			auto& next = node.rotations[r + 1];
			float delta_time = next.first - current.first;
			float factor = (time - current.first) / delta_time;
			ASSERT(factor >= 0.0f && factor <= 1.0f);
			return glm::mat4_cast(glm::normalize(glm::slerp(current.second, next.second, factor)));
		}
	}
	if (node.rotations.empty()) {
		return glm::mat4(1.0f);
	}
	return glm::mat4_cast(node.rotations.front().second);
}

glm::mat4 model_instance::next_interpolated_scale(int node_index, float time) {
	auto& node = source.animations[animation_index].channels[node_index];
	for (int s = animations[animation_index].channels[node_index].last_scale_key; s < (int)node.scales.size() - 1; s++) {
		if (node.scales[s + 1].first > time) {
			animations[animation_index].channels[node_index].last_scale_key = s;
			auto& current = node.scales[s];
			auto& next = node.scales[s + 1];
			float delta_time = next.first - current.first;
			float factor = (time - current.first) / delta_time;
			ASSERT(factor >= 0.0f && factor <= 1.0f);
			vector3f delta_scale = next.second - current.second;
			vector3f interpolated_delta = factor * delta_scale;
			vector3f scale = current.second + interpolated_delta;
			return glm::scale(glm::mat4(1.0f), { scale.x, scale.y, scale.z });
		}
	}
	if (node.scales.empty()) {
		return glm::mat4(1.0f);
	}
	auto& scale = node.scales.front().second;
	return glm::scale(glm::mat4(1.0f), { scale.x, scale.y, scale.z });
}

void model_instance::animate_node(int node_index, float time, const glm::mat4& transform) {
	auto& state = animations[animation_index].channels[node_index];
	auto& animation = source.animations[animation_index];
	glm::mat4 node_transform = source.nodes[node_index].transform;
	auto& node = animation.channels[node_index];
	if (node.positions.size() > 0 || node.rotations.size() > 0 || node.scales.size() > 0) {
		if (is_new_loop) {
			state.last_position_key = new_animation_transition_frame;
			state.last_rotation_key = new_animation_transition_frame;
			state.last_scale_key = new_animation_transition_frame;
		}
		glm::mat4 translation = next_interpolated_position(node_index, time);
		glm::mat4 rotation = next_interpolated_rotation(node_index, time);
		glm::mat4 scale = next_interpolated_scale(node_index, time);
		node_transform = translation * rotation * scale;
	}
	glm::mat4 new_transform = transform * node_transform;
	if (node.bone != -1) {
		// todo: there has to be a better way
		if (!my_attachment) {
			bones[node.bone] = source.root_transform * new_transform * source.bones[node.bone];
		} else {
			bones[node.bone] = new_transform * my_attachment->parent_bone * my_attachment->attachment_bone * source.root_transform;
		}
	}
	auto& children = source.nodes[node_index].children;
	for (int child : children) {
		animate_node(child, time, new_transform);
	}
	for (auto& attachment : state.attachments) {
		attachment.attachment.animate(new_transform);
	}
}

void model_instance::animate() {
	animate(glm::mat4(1.0f));
}

void model_instance::animate(const glm::mat4& transform) {
	auto& animation = source.animations[animation_index];
	double seconds = (double)animation_timer.milliseconds() * 0.001;
	double play_duration = seconds * (double)animation.ticks_per_second;
	is_new_loop = (play_duration >= (double)animation.duration) || is_new_animation;
	is_new_animation = false;
	float animation_time = (float)std::fmod(play_duration, (double)animation.duration);
	animate_node(0, animation_time, transform);
	new_animation_transition_frame = 0;
}

void model_instance::start_animation(int index) {
	if (index == animation_index) {
		return;
	}
	if (index < 0 || index >= source.total_animations()) {
		index = 0; // avoid crash
		WARNING("Invalid animation index passed: " << index);
	}
	new_animation_transition_frame = source.animations[animation_index].transitions[index];
	animation_index = index;
	animation_timer.start();
	// todo: fix this up, since nodes[2] is not guaranteed to have frames
	//float seconds = source.animations[animation_index].nodes[2].positions[new_animation_transition_frame].first;
	//float ms = seconds * 1000.0f;
	//animation_timer.go_back_in_time((long long)ms);
	is_new_animation = true;
}

void model_instance::draw() const {
	for (size_t b = 0; b < bones.size(); b++) {
		get_shader_variable("uni_Bones[" + std::to_string(b) + "]").set(bones[b]);
	}
	source.draw();
	for (size_t node_index : attachments) {
		auto& node = animations[animation_index].channels[node_index];
		for (auto& attachment : node.attachments) {
			attachment.attachment.draw();
		}
	}
}

int model_instance::attach(int parent, model& attachment_model, vector3f position, glm::quat rotation) {
	attachment_id_counter++;
	for (auto& animation : animations) {
		auto& channel = animation.channels[parent];
		glm::mat4 bone = source.bones[source.animations[0].channels[parent].bone];
		auto& attachment = channel.attachments.emplace_back(attachment_model, bone, position, rotation, parent, attachment_id_counter);
		attachment.attachment.my_attachment = &attachment;
		attachments.push_back(parent);
	}
	return attachment_id_counter;
}

bool model_instance::detach(int id) {
	// pretty.
	for (auto& animation : animations) {
		for (auto& node : animation.channels) {
			for (size_t i = 0; i < node.attachments.size(); i++) {
				if (node.attachments[i].id == id) {
					node.attachments.erase(node.attachments.begin() + i);
					return true;
				}
			}
		}
	}
	return false;
}

rectangle::rectangle() {
	set_tex_coords(0.0f, 0.0f, 1.0f, 1.0f);
}

void rectangle::set_tex_coords(float x, float y, float width, float height) {
	vertices.set({
		{ { 0.0f, 0.0f }, 1.0f, { x, y } },
		{ { 1.0f, 0.0f }, 1.0f, { x + width, y } },
		{ { 1.0f, 1.0f }, 1.0f, { x + height, y + height } },
		{ { 0.0f, 1.0f }, 1.0f, { x, y + height } }
	}, { 0, 1, 2, 3, 2, 0 });
}

void rectangle::bind() const {
	vertices.bind();
}

void rectangle::draw() const {
	vertices.draw();
}

}

std::ostream& operator<<(std::ostream& out, no::swap_interval interval) {
	switch (interval) {
	case no::swap_interval::late: return out << "Late";
	case no::swap_interval::immediate: return out << "Immediate";
	case no::swap_interval::sync: return out << "Sync";
	default: return out << "Unknown";
	}
}
