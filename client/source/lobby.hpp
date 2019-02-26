#pragma once

#include "client.hpp"
#include "packets.hpp"
#include "font.hpp"

class lobby_state : public client_state {
public:

	lobby_state();
	~lobby_state() override;

	void update() override;
	void draw() override;

private:

	int mouse_press_id = -1;
	int keyboard_press_id = -1;
	int receive_packet_id = -1;

	no::font font;

};
