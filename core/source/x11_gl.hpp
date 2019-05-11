#pragma once

#include "platform.hpp"

#if PLATFORM_LINUX && ENABLE_GL

#include "linux_platform.hpp"
#include "draw.hpp"

namespace no {

namespace platform {

class x11_gl_context {
public:

	bool exists() const;
	bool is_current() const;
	void make_current();
	void set_viewport(int x, int y, int width, int height);
	void set_clear_color(const vector3f& color);
	void set_scissor(int x, int y, int width, int height);
	bool set_swap_interval(int interval);
	bool set_swap_interval(swap_interval interval);
	void clear();
	void destroy();

	void log_renderer_info() const;

private:

	void initialize_gl();

};

}

}

#endif
