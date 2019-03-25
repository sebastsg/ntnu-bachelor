#pragma once

#include "draw.hpp"

namespace no {

struct bone_attachment {
	int parent = -1;
	glm::mat4 parent_bone;
	glm::mat4 child_bone;
	vector3f position;
	glm::quat rotation;

	void update();
};

struct skeletal_animation {
	bool active = true;
	timer play_timer;
	glm::mat4 bones[48];
	glm::mat4 transforms[48];
	glm::mat4 root_transform{ 1.0f };
	bool is_attachment = false;
	bone_attachment attachment;
	int bone_count = 0;
	int transform_count = 0;
	int loops_completed = 0;
	int loops_assigned = 0;
	int reference = -1;
	float time = 0.0f;
	transform3 transform;
	int next_p = 1;
	int next_r = 1;
	int next_s = 1;
};

class skeletal_animator {
public:
	
	struct {
		shader_variable bones;
	} shader;

	skeletal_animator(const model& skeleton);

	int add();
	void erase(int id);
	skeletal_animation& get(int id);
	int count() const;
	bool can_animate(int id) const;
	void reset(int id);
	bool will_be_reset(int id) const;

	void draw() const;
	void play(int id, int animation_index, int loops);
	void play(int id, const std::string& animation_name, int loops);
	void animate();

private:

	glm::mat4 next_interpolated_position(skeletal_animation& animation, int node);
	glm::mat4 next_interpolated_rotation(skeletal_animation& animation, int node);
	glm::mat4 next_interpolated_scale(skeletal_animation& animation, int node);
	void animate_node(skeletal_animation& animation, int parent, int node_index);
	void animate(skeletal_animation& animation);

	std::vector<skeletal_animation> animations;
	const model& skeleton;
	int active_count = 0;

};

struct bone_attachment_mapping {

	std::string root_model;
	std::string root_animation;
	std::string attached_model;
	std::string attached_animation;
	int attached_to_channel = -1;
	vector3f position;
	glm::quat rotation;

	bool is_same_mapping(const bone_attachment_mapping& that) const;
	std::string mapping_string() const;

};

class bone_attachment_mapping_list {
public:

	void save(const std::string& path);
	void load(const std::string& path);

	void for_each(const std::function<bool(bone_attachment_mapping&)>& handler);
	void remove_if(const std::function<bool(bone_attachment_mapping&)>& compare);

	bool exists(const bone_attachment_mapping& mapping);
	void add(const bone_attachment_mapping& mapping);
	bool update(const model& root, int animation_index, const std::string& attachment_model, bone_attachment& attachment) const;

	std::string find_root_animation(const std::string& root_model, const std::string& attached_model, const std::string& attached_animation) const;

private:

	std::vector<bone_attachment_mapping> mappings;

};

}
