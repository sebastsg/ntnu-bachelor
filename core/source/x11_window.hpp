#pragma once

#include "platform.hpp"

#if PLATFORM_LINUX

#include <X11/X.h>
#include <X11/Xlib-xcb.h>
#include <xcb/xcb.h>

#include "linux_platform.hpp"
#include "math.hpp"
#include "input.hpp"

#if ENABLE_GL
#include "linux_gl.hpp"
#endif

namespace no {

class window;

namespace platform {

class x11_window {
public:

	x11_window(window* window, const std::string& title, int width, int height, int samples, bool is_maximized);

	x11_window(const x11_window&) = delete;
	x11_window(x11_window&&) = delete;

	~x11_window();

	x11_window& operator=(const x11_window&) = delete;
	x11_window& operator=(x11_window&&) = delete;

	void poll();
	void set_base_window(window* window);

	bool is_open() const;
	vector2i position() const;
	vector2i size() const;

	std::string title() const;

	void set_size(const vector2i& size);
	void set_title(const std::string& title);
	void set_icon_from_resource(int resource_id);
	void set_cursor(mouse::cursor icon);
	void set_viewport(int x, int y, int width, int height);
	void set_scissor(int x, int y, int width, int height);
	void set_clear_color(const vector3f& color);
	bool set_swap_interval(swap_interval interval);

	void clear();
	void swap();

private:

    Display* display = nullptr;
    int default_screen = 0;
    xcb_connection_t* connection = nullptr;
    xcb_screen_t* screen = nullptr;

	window* base_window = nullptr;
	platform_render_context context;

};

}

}

#endif
