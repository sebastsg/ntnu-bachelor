#pragma once

#include "transform.hpp"
#include "event.hpp"
#include "loop.hpp"
#include "draw.hpp"
#include "font.hpp"

namespace no {

class ui_element {
public:

	transform2 transform;
	
	struct {
		signal_event click;
	} events;

	ui_element(window_state& state);
	ui_element(const ui_element&) = delete;
	ui_element(ui_element&&) = delete;

	virtual ~ui_element();

	ui_element& operator=(const ui_element&) = delete;
	ui_element& operator=(ui_element&&) = delete;

protected:
	
	window_state& state;

private:

	int mouse_press_id = -1;

};

class text_view : public ui_element {
public:

	text_view(window_state& state);
	text_view(const text_view&) = delete;
	text_view(text_view&&);

	~text_view();

	text_view& operator=(const text_view&) = delete;
	text_view& operator=(text_view&&);

	std::string text() const;
	void render(const no::font& font, const std::string& text);
	void draw(const no::rectangle& rectangle) const;

private:

	std::string rendered_text;
	int texture = -1;

};

class ui_button : public ui_element {
public:

	text_view label;
	shader_variable color;

	ui_button(window_state& state);
	ui_button(const ui_button&) = delete;
	ui_button(ui_button&&) = delete;

	~ui_button() override = default;

	ui_button& operator=(const ui_button&) = delete;
	ui_button& operator=(ui_button&&) = delete;

	void update();
	void draw();

private:

	void draw_button();
	void draw_label();

	struct {
		bool enabled = true;
		float in_speed = 0.2f;
		float out_speed = 0.1f;
		float current = 0.0f;
		float previous_frame = 1.0f;
		float next_frame = 0.0f;
	} transition;

	int sprite = -1;
	sprite_animation animation;
	align_type label_align = align_type::middle;
	vector2f label_padding;
	vector3f label_color = 1.0f;
	rectangle text_rectangle;

};

}
