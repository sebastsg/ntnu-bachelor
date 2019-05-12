#include "x11_window.hpp"

#if PLATFORM_LINUX

#include "debug.hpp"
#include "window.hpp"
#include "input.hpp"

namespace no {

namespace platform {

x11_window::x11_window(window* base_window, const std::string& title, int width, int height, int samples, bool is_maximized) {
    display = XOpenDisplay(nullptr);
    if (!display) {
		CRITICAL("Failed to open display.");
        return;
    }
    default_screen = DefaultScreen(display);
    connection = XGetXCBConnection(display);
    if (!connection) {
		CRITICAL("Failed to get XCB connection.");
        return;
    }
    XSetEventQueueOwner(display, XCBOwnsEventQueue);
    xcb_screen_iterator_t iterator = xcb_setup_roots_iterator(xcb_get_setup(connection));
    for (int i = default_screen; iterator.rem != 0 && i > 0; i--) {
        screen = iterator.data;
        xcb_screen_next(&iterator);
    }
}

windows_window::~windows_window() {
	XCloseDisplay(display);
}

void x11_window::poll() {
	
}

void x11_window::set_base_window(window* new_window) {
	base_window = new_window;
}

bool x11_window::is_open() const {
	return window_handle != nullptr;
}

vector2i x11_window::position() const {
	return 0;
}

vector2i x11_window::size() const {
	return 0;
}

std::string x11_window::title() const {
	return "";
}

void x11_window::set_size(const vector2i& size) {
	
}

void x11_window::set_title(const std::string& title) {
	
}

void x11_window::set_cursor(mouse::cursor icon) {
	
}

void x11_window::set_viewport(int x, int y, int width, int height) {
	context.set_viewport(x, y, width, height);
}

void x11_window::set_scissor(int x, int y, int width, int height) {
	context.set_scissor(x, y, width, height);
}

void x11_window::set_clear_color(const vector3f& color) {
	context.set_clear_color(color);
}

bool x11_window::set_swap_interval(swap_interval interval) {
	return context.set_swap_interval(interval);
}

void x11_window::clear() {
	context.clear();
}

void x11_window::swap() {
	
}

}

}

#endif
