#pragma once

#include "platform.hpp"

#if ENABLE_WINDOWS_GL

#include "windows_platform.hpp"
#include "draw.hpp"

#ifndef _WINDEF_
struct HGLRC__;
typedef HGLRC__ *HGLRC;
#endif

namespace no {

namespace platform {

class windows_gl_context {
public:

	void create_dummy(HDC device_context_handle);
	void create_with_attributes(HDC device_context_handle, int samples);
	HGLRC handle() const;
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

	static HGLRC current_context_handle();

private:

	void initialize_glew();
	void initialize_gl();

	HGLRC gl_context = nullptr;
	HDC device_context_handle = nullptr;
	bool is_arb_context = false;

};

}

}

#endif
