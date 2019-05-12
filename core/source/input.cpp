#include "input.hpp"

#if ENABLE_WINDOW

#include "window.hpp"

#if PLATFORM_WINDOWS
#include <Windows.h>
#include "windows_window.hpp"
#endif

namespace no {

mouse::mouse(window* parent_window) : parent_window(parent_window) {

}

int mouse::x() const {
	return position().x;
}

int mouse::y() const {
	return position().y;
}

vector2i mouse::position() const {
#if PLATFORM_WINDOWS && ENABLE_WINDOW
	POINT cursor;
	GetCursorPos(&cursor);
	ScreenToClient(parent_window->platform_window()->handle(), &cursor);
	return { cursor.x, cursor.y };
#endif
	return 0; // todo: implement
}

bool mouse::is_button_down(button button) const {
#if PLATFORM_WINDOWS && ENABLE_WINDOW
	switch (button) {
	case button::left:
		return (GetAsyncKeyState(VK_LBUTTON) & 0b1000000000000000) == 0b1000000000000000;
	case button::middle:
		return (GetAsyncKeyState(VK_MBUTTON) & 0b1000000000000000) == 0b1000000000000000;
	case button::right:
		return (GetAsyncKeyState(VK_RBUTTON) & 0b1000000000000000) == 0b1000000000000000;
	default:
		return false;
	}
#endif
	return false; // todo: implement
}

void mouse::set_icon(cursor icon) {
	parent_window->set_cursor(icon);
}

keyboard::keyboard() {
	std::fill(std::begin(keys), std::end(keys), false);
	press.listen([this](key pressed_key) {
		keys[(size_t)pressed_key] = true;
	});
	release.listen([this](key released_key) {
		keys[(size_t)released_key] = false;
	});
}

bool keyboard::is_key_down(key check_key) const {
	return keys[(size_t)check_key];
}

}

std::ostream& operator<<(std::ostream& out, no::mouse::button button) {
	switch (button) {
	case no::mouse::button::none: return out << "None";
	case no::mouse::button::left: return out << "Left";
	case no::mouse::button::middle: return out << "Middle";
	case no::mouse::button::right: return out << "Right";
	default: return out << "Unknown";
	}
}

std::ostream& operator<<(std::ostream& out, no::key key) {
	switch (key) {
	case no::key::backspace:
		return out << "Backspace";
	case no::key::tab:
		return out << "Tab";
	case no::key::enter:
		return out << "Enter";
	case no::key::pause:
		return out << "Pause";
	case no::key::caps_lock:
		return out << "Caps Lock";
	case no::key::escape:
		return out << "Escape";
	case no::key::space:
		return out << "Space";
	case no::key::left:
		return out << "Left";
	case no::key::up:
		return out << "Up";
	case no::key::right:
		return out << "Right";
	case no::key::down:
		return out << "Down";
	case no::key::print_screen:
		return out << "Print Screen";
	case no::key::del:
		return out << "Delete";
	case no::key::num_0:
	case no::key::num_1:
	case no::key::num_2:
	case no::key::num_3:
	case no::key::num_4:
	case no::key::num_5:
	case no::key::num_6:
	case no::key::num_7:
	case no::key::num_8:
	case no::key::num_9:
		return out << (char)key;
	case no::key::a:
	case no::key::b:
	case no::key::c:
	case no::key::d:
	case no::key::e:
	case no::key::f:
	case no::key::g:
	case no::key::h:
	case no::key::i:
	case no::key::j:
	case no::key::k:
	case no::key::l:
	case no::key::m:
	case no::key::n:
	case no::key::o:
	case no::key::p:
	case no::key::q:
	case no::key::r:
	case no::key::s:
	case no::key::t:
	case no::key::u:
	case no::key::v:
	case no::key::w:
	case no::key::x:
	case no::key::y:
	case no::key::z:
		return out << (char)key;
	case no::key::f1:
	case no::key::f2:
	case no::key::f3:
	case no::key::f4:
	case no::key::f5:
	case no::key::f6:
	case no::key::f7:
	case no::key::f8:
	case no::key::f9:
	case no::key::f10:
	case no::key::f11:
	case no::key::f12:
		return out << 'F' << (int)key - (int)no::key::f1 + 1;
	case no::key::num_lock:
		return out << "Num Lock";
	case no::key::scroll_lock:
		return out << "Scroll Lock";
	case no::key::left_shift:
		return out << "Left Shift";
	case no::key::right_shift:
		return out << "Right Shift";
	case no::key::left_control:
		return out << "Left Control";
	case no::key::right_control:
		return out << "Right Control";
	default:
		return out << "Unknown (" << (char)key << ")";
	}
}

#endif
