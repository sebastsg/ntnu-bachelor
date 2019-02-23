#pragma once

#include "loop.hpp"

class menu_bar_state : public no::window_state {
public:

	void update() override;
	void draw() override;

protected:

	virtual void update_menu();

private:

	bool limit_fps = true;

};

