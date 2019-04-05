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
	void equipment_type_combo(equipment_type& slot, const std::string& ui_id);

	void ui_create_item();
	void ui_select_item();

	int current_item = -1;
	int ui_texture = -1;

	item_definition new_item;

};
