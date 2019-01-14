#pragma once

#include "event.hpp"
#include "math.hpp"

namespace no {

class window;

class mouse {
public:

	enum class button { none, left, middle, right };

	struct move_message {
		vector2i relative;
		vector2i position;
	};

	struct press_message {
		button button = button::none;
	};

	struct release_message {
		button button = button::none;
	};

	struct double_click_message {
		button button = button::none;
	};

	struct scroll_message {
		int steps = 0;
	};

	struct visibility_message {
		bool is_visible = true;
	};

	message_event<move_message> move;
	message_event<press_message> press;
	message_event<release_message> release;
	message_event<double_click_message> double_click;
	message_event<scroll_message> scroll;
	message_event<visibility_message> visibility;
	signal_event cursor;

	mouse(window* parent_window);
	mouse() = default;

	int x() const;
	int y() const;
	vector2i position() const;
	bool is_button_down(button button) const;

private:

	window* parent_window = nullptr;

};

enum class key {

	unknown = 0,

	backspace = 8,
	tab = 9,
	enter = 13,
	pause = 19,
	caps_lock = 20,
	escape = 27,
	space = 32,
	left = 37,
	up = 38,
	right = 39,
	down = 40,
	print_screen = 44,
	del = 46,

	num_0 = 48, // '0'
	num_1 = 49,
	num_2 = 50,
	num_3 = 51,
	num_4 = 52,
	num_5 = 53,
	num_6 = 54,
	num_7 = 55,
	num_8 = 56,
	num_9 = 57,

	a = 65, // 'A'
	b = 66,
	c = 67,
	d = 68,
	e = 69,
	f = 70,
	g = 71,
	h = 72,
	i = 73,
	j = 74,
	k = 75,
	l = 76,
	m = 77,
	n = 78,
	o = 79,
	p = 80,
	q = 81,
	r = 82,
	s = 83,
	t = 84,
	u = 85,
	v = 86,
	w = 87,
	x = 88,
	y = 89,
	z = 90,

	f1 = 112,
	f2 = 113,
	f3 = 114,
	f4 = 115,
	f5 = 116,
	f6 = 117,
	f7 = 118,
	f8 = 119,
	f9 = 120,
	f10 = 121,
	f11 = 122,
	f12 = 123,

	num_lock = 144,
	scroll_lock = 145,

	left_shift = 160,
	right_shift = 161,

	left_control = 162,
	right_control = 163,

	plus = 187,
	minus = 189,

	max_keys,

};

class keyboard {
public:

	struct press_message {
		key key;
	};

	struct repeated_press_message {
		key key;
		int repeat = 0;
	};

	struct release_message {
		key key;
	};

	struct input_message {
		unsigned int character = 0;
	};

	message_event<press_message> press;
	message_event<repeated_press_message> repeated_press;
	message_event<release_message> release;
	message_event<input_message> input;

	keyboard();

	bool is_key_down(key key) const;

private:

	bool keys[(size_t)key::max_keys];

};

}

std::ostream& operator<<(std::ostream& out, no::mouse::button button);
std::ostream& operator<<(std::ostream& out, no::key key);
