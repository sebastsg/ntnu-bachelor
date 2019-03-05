#pragma once

#include "menu.hpp"
#include "quest.hpp"
#include "common_ui.hpp"

class quest_editor : public menu_bar_state {
public:

	quest_editor();
	~quest_editor() override;

	void update() override;
	void draw() override;

private:

	void begin_window(const std::string& label, float width);
	void end_window();

	void create_new_quest();
	void load_quest(int index);
	void save_quest();
	
	void update_quest_list();
	void update_tasks();
	void update_hints();

	bool dirty = false;
	int selected_quest_index = -1;

	float next_window_x = 0.0f;
	int blank_texture = -1;

};

