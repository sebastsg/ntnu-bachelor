#pragma once

#include "event.hpp"
#include "platform.hpp"
#include "math.hpp"
#include "input.hpp"
#include "draw.hpp"

namespace no {

class window {
public:

	enum class display_mode { windowed, fullscreen, fullscreen_desktop };

	struct resize_message {
		vector2i size;
	};

	signal_event close;
	message_event<resize_message> resize;

	keyboard keyboard;
	mouse mouse;

	window(const std::string& title, int width, int height, int samples, bool maximized);
	window(const std::string& title);
	window(const window&) = delete;
	window(window&&);
	~window();

	window& operator=(const window&) = delete;
	window& operator=(window&&) = delete;

	void poll();

	void clear();
	void swap();

	void maximize();

	void set_title(const std::string& title);
	void set_icon_from_resource(int resource_id);
	void set_width(int width);
	void set_height(int height);
	void set_size(const vector2i& size);
	void set_display_mode(display_mode mode);
	void set_clear_color(const vector3f& color);

	std::string title() const;
	int width() const;
	int height() const;
	vector2i size() const;
	display_mode current_display_mode() const;

	int x() const;
	int y() const;
	vector2i position() const;
	bool is_open() const;

	bool set_swap_interval(swap_interval interval);
	void set_viewport(int x, int y, int width, int height);
	void scissor(int x, int y, int width, int height);
	void reset_scissor();

	platform::platform_window* platform_window();

private:

	platform::platform_window* platform = nullptr;
	display_mode last_set_display_mode = display_mode::windowed;

};

}

std::ostream& operator<<(std::ostream& out, no::window::display_mode mode);
