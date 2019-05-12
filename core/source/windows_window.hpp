#pragma once

#include "platform.hpp"

#if PLATFORM_WINDOWS && ENABLE_WINDOW

#include "windows_platform.hpp"
#include "math.hpp"
#include "input.hpp"

#if ENABLE_GL
#include "windows_gl.hpp"
#endif

namespace no {

class window;

namespace platform {

class windows_window {
public:

	windows_window(window* window, const std::string& title, int width, int height, int samples, bool is_maximized);

	windows_window(const windows_window&) = delete;
	windows_window(windows_window&&) = delete;

	~windows_window();

	windows_window& operator=(const windows_window&) = delete;
	windows_window& operator=(windows_window&&) = delete;

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

	HWND handle() const;

private:

	HWND window_handle = nullptr;
	HDC device_context_handle = nullptr;
	window* base_window = nullptr;
	platform_render_context context;

};

}

}

#endif
