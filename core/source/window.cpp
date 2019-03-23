#include "window.hpp"
#include "windows_window.hpp"
#include "debug.hpp"
#include "timer.hpp"

namespace no {

window::window(const std::string& title, int width, int height, int samples, bool maximized) : mouse(this) {
	platform = new platform::platform_window(this, title, width, height, samples, maximized);
}

window::window(const std::string& title) : mouse(this) {
	platform = new platform::platform_window(this, title, 0, 0, 1, true);
}

window::window(window&& that) {
	last_set_display_mode = that.last_set_display_mode;
	close = std::move(that.close);
	resize = std::move(that.resize);
	platform = that.platform;
	platform->set_base_window(this);
	mouse = { this };
	that.platform = nullptr;
	that.mouse = { nullptr };
}

window::~window() {
	delete platform;
}

void window::poll() {
	platform->poll();
}

void window::clear() {
	platform->clear();
}

void window::swap() {
	platform->swap();
}

void window::maximize() {
	// todo: maximize
	set_viewport(0, 0, width(), height());
}

void window::set_title(const std::string& title) {
	platform->set_title(title);
}

void window::set_icon_from_resource(int resource_id) {
	platform->set_icon_from_resource(resource_id);
}

void window::set_cursor(mouse::cursor icon) {
	platform->set_cursor(icon);
}

void window::set_width(int width) {
	set_size({ width, height() });
}

void window::set_height(int height) {
	set_size({ width(), height });
}

void window::set_size(const vector2i& size) {
	platform->set_size(size);
}

void window::set_display_mode(display_mode mode) {
	// todo: change mode
	switch (mode) {
	case display_mode::windowed:
		break;
	case display_mode::fullscreen:
		break;
	case display_mode::fullscreen_desktop:
		break;
	default:
		WARNING("Window mode not found: " << mode);
		return;
	}
	last_set_display_mode = mode;
}

void window::set_clear_color(const vector3f& color) {
	platform->set_clear_color(color);
}

std::string window::title() const {
	return platform->title();
}

int window::width() const {
	return size().x;
}

int window::height() const {
	return size().y;
}

vector2i window::size() const {
	return platform->size();
}

window::display_mode window::current_display_mode() const {
	return last_set_display_mode;
}

int window::x() const {
	return position().x;
}

int window::y() const {
	return position().y;
}

vector2i window::position() const {
	return platform->position();
}

bool window::is_open() const {
	return platform->is_open();
}

bool window::set_swap_interval(swap_interval interval) {
	return platform->set_swap_interval(interval);
}

void window::set_viewport(int x, int y, int width, int height) {
	platform->set_viewport(x, y, width, height);
}

void window::scissor(int x, int y, int w, int h) {
	platform->set_scissor(x, height() - y - h, w, h);
}

void window::reset_scissor() {
	platform->set_scissor(0, 0, width(), height());
}

platform::platform_window* window::platform_window() {
	return platform;
}

}

std::ostream& operator<<(std::ostream& out, no::window::display_mode mode) {
	switch (mode) {
	case no::window::display_mode::windowed: return out << "Windowed";
	case no::window::display_mode::fullscreen: return out << "Fullscreen";
	case no::window::display_mode::fullscreen_desktop: return out << "Fullscreen Desktop";
	default: return out << "Unknown";
	}
}
