#pragma once

#include "loop.hpp"
#include "common_ui.hpp"

class menu_bar_state : public no::program_state {
public:

	void update() override;
	void draw() override;

protected:

	virtual void update_menu();

private:

	bool limit_fps = true;

};

