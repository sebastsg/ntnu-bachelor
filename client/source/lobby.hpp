#pragma once

#include "client.hpp"
#include "packets.hpp"
#include "camera.hpp"
#include "ui.hpp"

class lobby_state : public client_state {
public:

	lobby_state();
	~lobby_state() override;

	void update() override;
	void draw() override;

private:

	void save_config();
	void load_config();

	int mouse_press_id = -1;
	int keyboard_press_id = -1;
	int receive_packet_id = -1;

	no::ortho_camera camera;
	int shader = -1;
	no::shader_variable color;
	no::font font;
	int button_texture = -1;

	no::text_view login_label;
	no::button login_button;
	no::rectangle rectangle;

	no::input_field username;
	no::input_field password;
	int input_background = -1;

	no::button remember_username;
	bool remember_username_check = false;

};
