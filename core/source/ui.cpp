#include "ui.hpp"
#include "input.hpp"

namespace no {

ui_element::ui_element(window_state& state_) : state(state_) {
	mouse_press_id = state.mouse().press.listen([this](const mouse::press_message& event) {
		if (event.button != mouse::button::left) {
			return;
		}
		if (transform.collides_with(state.mouse().position().to<float>())) {
			events.click.emit();
		}
	});
}

ui_element::~ui_element() {
	state.mouse().press.ignore(mouse_press_id);
}

text_view::text_view(window_state& state) : ui_element(state) {
	texture = create_texture();
}

text_view::text_view(text_view&& that) : ui_element(that.state) {
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

void text_view::render(const no::font& font, const std::string& text) {
	if (text == rendered_text) {
		return;
	}
	rendered_text = text;
	load_texture(texture, font.render(rendered_text));
	transform.scale = texture_size(texture).to<float>();
}

void text_view::draw(const no::rectangle& rectangle) const {
	bind_texture(texture);
	draw_shape(rectangle, transform);
}

ui_button::ui_button(window_state& state) : ui_element(state), label(state) {
	animation.pause();
}

void ui_button::update() {
	if (transform.collides_with(state.mouse().position().to<float>())) {
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
	label.transform.align(label_align, transform, label_padding);
}

void ui_button::draw() {
	draw_button();
	draw_label();
}

void ui_button::draw_button() {
	bind_texture(sprite);
	if (transition.enabled) {
		color.set({ 1.0f, 1.0f, 1.0f, 1.0f });
		animation.set_frame((int)transition.previous_frame);
		animation.draw(transform);
		color.set({ 1.0f, 1.0f, 1.0f, transition.current });
		animation.set_frame((int)transition.next_frame);
		animation.draw(transform);
	} else {
		animation.draw(transform);
	}
}

void ui_button::draw_label() {
	if (label.text().empty()) {
		return;
	}
	color.set({ label_color.x, label_color.y, label_color.z, 1.0f });
	label.draw(text_rectangle);
	if (transition.enabled) {
		color.set({ label_color.x, label_color.y, label_color.z, transition.current });
		label.draw(text_rectangle);
	}
}

}
