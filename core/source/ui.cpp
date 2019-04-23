#include "ui.hpp"
#include "input.hpp"

namespace no {

ui_element::ui_element(const window_state& state_, const ortho_camera& camera_) : state(state_), camera(camera_) {
	mouse_release_id = state.mouse().release.listen([this](const mouse::release_message& event) {
		if (event.button == mouse::button::left && transform.collides_with(camera.mouse_position(state.mouse()))) {
			events.click.emit();
		}
	});
}

ui_element::~ui_element() {
	state.mouse().release.ignore(mouse_release_id);
}

bool ui_element::is_hovered() const {
	return transform.collides_with(camera.mouse_position(state.mouse()));
}

bool ui_element::is_pressed() const {
	return is_hovered() && state.mouse().is_button_down(mouse::button::left);
}

text_view::text_view(const window_state& state, const ortho_camera& camera) : ui_element(state, camera) {
	texture = create_texture();
}

text_view::text_view(text_view&& that) : ui_element(that.state, that.camera) {
	std::swap(transform, that.transform);
	std::swap(rendered_text, that.rendered_text);
	std::swap(texture, that.texture);
}

text_view::~text_view() {
	delete_texture(texture);
}

text_view& text_view::operator=(text_view&& that) {
	std::swap(transform, that.transform);
	std::swap(rendered_text, that.rendered_text);
	std::swap(texture, that.texture);
	return *this;
}

std::string text_view::text() const {
	return rendered_text;
}

void text_view::render(const font& font, const std::string& text) {
	if (text == rendered_text) {
		return;
	}
	rendered_text = text;
	load_texture(texture, font.render(rendered_text));
	transform.scale = texture_size(texture).to<float>();
}

void text_view::draw(const rectangle& rectangle) const {
	bind_texture(texture);
	draw_shape(rectangle, transform);
}

button::button(const window_state& state, const ortho_camera& camera) : ui_element(state, camera), label(state, camera) {
	animation.pause();
	animation.frames = 3;
}

void button::update() {
	if (is_hovered()) {
		if (transition.enabled) {
			transition.current += transition.in_speed;
			if (transition.current > 1.0f) {
				transition.current = 1.0f;
			}
			if (state.mouse().is_button_down(mouse::button::left)) {
				transition.previous_frame = 1.0f;
				transition.next_frame = 2.0f;
			} else {
				transition.previous_frame = 0.0f;
				transition.next_frame = 1.0f;
			}
		} else {
			animation.set_frame(1);
			if (state.mouse().is_button_down(mouse::button::left)) {
				animation.set_frame(2);
			}
		}
	} else {
		if (transition.enabled) {
			transition.current -= transition.out_speed;
			if (transition.current < 0.0f) {
				transition.current = 0.0f;
			}
			transition.previous_frame = 0.0f;
			transition.next_frame = 1.0f;
		} else {
			animation.set_frame(0);
		}
	}
	label.transform.align(label_alignment, transform, label_padding);
}

void button::draw_button() {
	if (transition.enabled) {
		color.set({ 1.0f, 1.0f, 1.0f, 1.0f });
		animation.set_frame((int)transition.previous_frame);
		animation.draw(transform);
		color.set({ 1.0f, 1.0f, 1.0f, transition.current });
		animation.set_frame((int)transition.next_frame);
	}
	animation.draw(transform);
}

void button::draw_label() {
	if (label.text().empty()) {
		return;
	}
	color.set({ label_color.x, label_color.y, label_color.z, 1.0f });
	label.draw(text_rectangle);
	if (transition.enabled) {
		color.set({ label_hover_color.x, label_hover_color.y, label_hover_color.z, transition.current });
		label.draw(text_rectangle);
	}
}

void button::set_tex_coords(vector2f position, vector2f size) {
	animation.set_tex_coords(position, size);
}

input_field::input_field(const window_state& state_, const ortho_camera& camera_, const font& font) : ui_element(state_, camera_), label(state_, camera_), input_font(font) {
	animation.pause();
	animation.frames = 3;
	events.click.listen([this] {
		focus();
	});
	mouse_press = state.mouse().press.listen([this](const mouse::press_message& event) {
		if (event.button == mouse::button::left) {
			if (transform.collides_with(camera.mouse_position(state.mouse()))) {
				focus();
			} else {
				blur();
			}
		}
	});
}

input_field::~input_field() {
	blur();
	state.mouse().press.ignore(mouse_press);
}

void input_field::update() {
	if (key_input != -1) {
		animation.set_frame(2);
	} else if (transform.collides_with(camera.mouse_position(state.mouse()))) {
		if (state.mouse().is_button_down(mouse::button::left)) {
			animation.set_frame(2);
		} else {
			animation.set_frame(1);
		}
	} else {
		animation.set_frame(0);
	}
	label.transform.align(align_type::left, transform, padding);
	std::string text = censor ? std::string(input.size(), '*') : input;
	label.render(input_font, key_input == -1 ? text : text + "|");
}

void input_field::draw(int sprite) {
	draw_background(sprite);
	draw_text();
}

void input_field::focus() {
	if (key_input != -1) {
		return;
	}
	key_input = state.keyboard().input.listen([this](const keyboard::input_message& event) {
		if (event.character == (unsigned int)key::enter) {
			return;
		}
		if (event.character == (unsigned int)key::backspace) {
			if (!input.empty()) {
				input = input.substr(0, input.size() - 1);
			}
		} else if (event.character < 0x7F) {
			input += (char)event.character;
		}
	});
}

void input_field::blur() {
	state.keyboard().input.ignore(key_input);
	key_input = -1;
}

void input_field::draw_background(int sprite) {
	bind_texture(sprite);
	animation.draw(transform);
}

void input_field::draw_text() {
	if (!label.text().empty()) {
		label.draw(text_rectangle);
	}
}

std::string input_field::value() const {
	return input;
}

void input_field::set_value(const std::string& value) {
	input = value;
}

}
