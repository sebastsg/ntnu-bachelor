#pragma once

#include "menu.hpp"
#include "object.hpp"

class object_editor : public menu_bar_state {
public:

	object_editor();
	~object_editor() override;

	void update() override;
	void draw() override;

private:



};
