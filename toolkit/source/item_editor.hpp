#pragma once

#include "menu.hpp"
#include "item.hpp"

class item_editor : public menu_bar_state {
public:

	item_editor();
	~item_editor() override;

	void update() override;
	void draw() override;

private:

	void item_type_combo(item_type& type, const std::string& ui_id);
	void equipment_slot_combo(equipment_slot& slot, const std::string& ui_id);

	void ui_create_item();
	void ui_select_item();

	item_definition_list items;

	int current_item = -1;

	struct {
		char name[100] = {};
		int max_stack = 1;
		item_type type = item_type::other;
		equipment_slot slot = equipment_slot::none;
	} new_item;

};

