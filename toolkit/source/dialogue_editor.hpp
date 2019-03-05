#pragma once

#include "menu.hpp"
#include "draw.hpp"
#include "dialogue.hpp"

struct ImDrawList;

class dialogue_editor_state : public menu_bar_state {
public:

	dialogue_editor_state();
	~dialogue_editor_state() override;

	void update() override;
	void draw() override;

private:

	struct link_circle {
		no::vector2f position;
		int bezier_index = 0;
		float t = 0.0f;
		link_circle() = default;
		link_circle(const no::vector2f& position, int bezier, float t)
			: position(position), bezier_index(bezier), t(t) {
		}
	};

	std::unordered_map<abstract_node*, std::vector<link_circle>> node_circles;

	int selected_dialogue_index = -1;

	dialogue_tree dialogue;

	int node_index_link_from = -1;
	int node_link_from_out = -1;
	int node_selected = -1;
	int node_index_hovered = -1;
	bool can_show_context_menu = false;
	bool open_context_menu = false;
	no::vector2f scrolling;
	
	int ui_texture = -1;
	int blank_texture = -1;

	bool dirty = false;
	
	void update_header();
	void update_dialogue_list();
	void update_selected_dialogue();
	void update_node_links(ImDrawList* draw_list, no::vector2f offset);
	void update_nodes(ImDrawList* draw_list, no::vector2f offset);
	void update_context_menu(no::vector2f offset);
	void update_scrolling();

	void destroy_current_dialogue();
	void create_new_dialogue();
	void load_dialogue(int index);
	void save_dialogue();

	bool item_popup_context(std::string label, item_instance* out_item);
	bool select_stat_combo(int* stat);
	bool select_quest_combo(int* quest_id);
	bool select_quest_task_combo(int quest_id, int* task_id);

	void update_node_ui(message_node& node);
	void update_node_ui(choice_node& node);
	void update_node_ui(has_item_condition_node& node);
	void update_node_ui(stat_condition_node& node);
	void update_node_ui(inventory_effect_node& node);
	void update_node_ui(stat_effect_node& node);
	void update_node_ui(warp_effect_node& node);
	void update_node_ui(var_condition_node& node);
	void update_node_ui(modify_var_node& node);
	void update_node_ui(create_var_node& node);
	void update_node_ui(var_exists_node& node);
	void update_node_ui(delete_var_node& node);
	void update_node_ui(random_node& node);
	void update_node_ui(random_condition_node& node);
	void update_node_ui(quest_task_condition_node& node);
	void update_node_ui(quest_done_condition_node& node);
	void update_node_ui(quest_update_task_effect_node& node);

};
