#pragma once

#include "platform.hpp"

#if ENABLE_GRAPHICS

#include "draw.hpp"

#include <thread>
#include <mutex>
#include <atomic>

namespace no {

struct bone_attachment {

	int parent = -1;
	glm::mat4 parent_bone;
	glm::mat4 child_bone;
	vector3f position;
	glm::quat rotation;

	void update();

};

struct skeletal_animation_update {

	bool is_attachment = false;
	glm::mat4 root_transform{ 1.0f };
	bone_attachment attachment;

	int reference = -1;
	bool reset = false;
	int loops_if_reset = 0;

};

struct skeletal_animation {

	bool active = true;
	int id = -1;
	int reference = -1;
	bone_attachment attachment;
	float time = 0.0f;
	timer play_timer;
	glm::mat4 bones[48];
	glm::mat4 transforms[48];
	glm::mat4 root_transform{ 1.0f };
	bool is_attachment = false;
	int bone_count = 0;
	int transform_count = 0;
	int loops_completed = 0;
	int loops_assigned = 0;
	int next_p = 1;
	int next_r = 1;
	int next_s = 1;

	skeletal_animation(int id);

	void apply_update(skeletal_animation_update& update);

};

struct synced_skeletal_animation {

	bool active = true;
	int id = -1;
	int reference = -1;
	bone_attachment attachment;
	glm::mat4 bones[48];
	glm::mat4 transforms[48];
	glm::mat4 root_transform{ 1.0f };
	int bone_count = 0;
	int transform_count = 0;
	no::timer played_for;

	synced_skeletal_animation(const skeletal_animation& animation);

};

class skeletal_animator {
public:
	
	struct {
		shader_variable bones;
	} shader;

	skeletal_animator(const model& skeleton);
	skeletal_animator(const skeletal_animator&) = delete;
	skeletal_animator(skeletal_animator&&) = delete;

	~skeletal_animator();

	skeletal_animator& operator=(const skeletal_animator&) = delete;
	skeletal_animator& operator=(skeletal_animator&&) = delete;

	int add();
	void erase(int id);
	void set_transform(int id, const no::transform3& transform);
	void set_is_attachment(int id, bool attachment);
	void set_root_transform(int id, const glm::mat4& transform);
	int count() const;
	bool can_animate(int id) const;
	void reset(int id);
	bool will_be_reset(int id) const;
	glm::mat4 get_transform(int id, int transform) const;
	void set_attachment_bone(int id, const bone_attachment& attachment);
	bone_attachment get_attachment_bone(int id) const;

	void animate();
	void sync();

	void draw() const;
	void play(int id, int animation_index, int loops);
	void play(int id, const std::string& animation_name, int loops);

private:

	glm::mat4 next_interpolated_position(skeletal_animation& animation, int node);
	glm::mat4 next_interpolated_rotation(skeletal_animation& animation, int node);
	glm::mat4 next_interpolated_scale(skeletal_animation& animation, int node);
	void animate_node(skeletal_animation& animation, int parent, int node_index);
	void animate(skeletal_animation& animation);
	
	std::mutex animating;
	mutable std::mutex reading_synced;

	std::vector<transform3> model_transforms;
	std::vector<skeletal_animation> animations;
	std::vector<synced_skeletal_animation> synced_animations;
	std::vector<skeletal_animation_update> animation_updates;
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

#endif
