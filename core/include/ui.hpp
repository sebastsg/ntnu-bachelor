#pragma once

#include "platform.hpp"

#if ENABLE_GRAPHICS

#include "transform.hpp"
#include "event.hpp"
#include "loop.hpp"
#include "draw.hpp"
#include "font.hpp"
#include "camera.hpp"

namespace no {

class ui_element {
public:

	transform2 transform;
	
	struct {
		signal_event click;
	} events;

	ui_element(const program_state& state, const ortho_camera& camera);
	ui_element(const ui_element&) = delete;
	ui_element(ui_element&&) = delete;

	virtual ~ui_element();

	ui_element& operator=(const ui_element&) = delete;
	ui_element& operator=(ui_element&&) = delete;

	bool is_hovered() const;
	bool is_pressed() const;

protected:
	
	const program_state& state;
	const ortho_camera& camera;

private:

	int mouse_release_id = -1;

};

class text_view : public ui_element {
public:

	text_view(const program_state& state, const ortho_camera& camera);
	text_view(const text_view&) = delete;
	text_view(text_view&&);

	~text_view();

	text_view& operator=(const text_view&) = delete;
	text_view& operator=(text_view&&);

	std::string text() const;
	void render(const font& font, const std::string& text);
	void draw(const rectangle& rectangle) const;

private:

	std::string rendered_text;
	int texture = -1;

};

class button : public ui_element {
public:

	text_view label;
	shader_variable color;
	vector3f label_color;
	vector3f label_hover_color = 1.0f;
	align_type label_alignment = align_type::middle;
	vector2f label_padding;

	button(const program_state& state, const ortho_camera& camera);
	button(const button&) = delete;
	button(button&&) = delete;

	~button() override = default;

	button& operator=(const button&) = delete;
	button& operator=(button&&) = delete;

	void update();
	void draw_button();
	void draw_label();
	
	void set_tex_coords(vector2f position, vector2f size);

	struct {
		bool enabled = true;
		float in_speed = 0.2f;
		float out_speed = 0.1f;
		float current = 0.0f;
		float previous_frame = 1.0f;
		float next_frame = 0.0f;
	} transition;

	sprite_animation animation;
	rectangle text_rectangle;

};

class input_field : public ui_element {
public:

	bool censor = false;
	vector2f padding = { 8.0f, 0.0f };

	input_field(const program_state& state, const ortho_camera& camera, const font& font);
	input_field(const input_field&) = delete;
	input_field(input_field&&) = delete;

	~input_field() override;

	input_field& operator=(const input_field&) = delete;
	input_field& operator=(input_field&&) = delete;

	void update();
	void draw(int sprite);

	void focus();
	void blur();

	std::string value() const;
	void set_value(const std::string& value);

private:

	void draw_background(int sprite);
	void draw_text();
	
	sprite_animation animation;
	rectangle text_rectangle;
	const font& input_font;
	text_view label;
	std::string input;

	int key_input = -1;
	int mouse_press = -1;

};

}

#endif
