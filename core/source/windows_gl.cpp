#include "windows_gl.hpp"

#if ENABLE_WINDOWS_GL

#include <glew/glew.h>
#include <glew/wglew.h>

#include "debug.hpp"

namespace no {

namespace platform {

static void set_pixel_format(HDC device_context_handle) {
	MESSAGE("Setting pixel format");
	PIXELFORMATDESCRIPTOR descriptor = {};
	descriptor.nSize = sizeof(descriptor);
	descriptor.nVersion = 1;
	descriptor.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	descriptor.iLayerType = PFD_MAIN_PLANE;
	descriptor.iPixelType = PFD_TYPE_RGBA;
	descriptor.cColorBits = 32;
	int format = ChoosePixelFormat(device_context_handle, &descriptor);
	if (format == 0) {
		CRITICAL("Did not find suitable pixel format");
		return;
	}
	DescribePixelFormat(device_context_handle, format, sizeof(PIXELFORMATDESCRIPTOR), &descriptor);
	auto status = SetPixelFormat(device_context_handle, format, &descriptor);
	if (!status) {
		CRITICAL("Failed to set pixel format");
	}
}

static void set_pixel_format_arb(HDC device_context_handle, int samples) {
	switch (samples) {
	case 1:
	case 2:
	case 4:
	case 8:
	case 16:
		break;
	default:
		samples = 1;
		break;
	}
	MESSAGE("Setting pixel format using the ARB extension");
	const int int_attributes[] = {
		WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
		WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
		WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
		WGL_SAMPLE_BUFFERS_ARB, GL_TRUE,
		WGL_SAMPLES_ARB, samples,
		WGL_ACCELERATION_ARB, WGL_FULL_ACCELERATION_ARB,
		WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
		WGL_COLOR_BITS_ARB, 24,
		WGL_ALPHA_BITS_ARB, 8,
		WGL_DEPTH_BITS_ARB, 16,
		WGL_STENCIL_BITS_ARB, 0,
		0
	};
	const float float_attributes[] = {
		0
	};
	int format = 0;
	unsigned int count = 0;
	bool success = wglChoosePixelFormatARB(device_context_handle, int_attributes, float_attributes, 1, &format, &count);
	if (!success || count == 0) {
		WARNING("Failed to find pixel format");
		return;
	}
	PIXELFORMATDESCRIPTOR descriptor = {};
	DescribePixelFormat(device_context_handle, format, sizeof(PIXELFORMATDESCRIPTOR), &descriptor);
	auto status = SetPixelFormat(device_context_handle, format, &descriptor);
	if (!status) {
		CRITICAL("Failed to set pixel format");
	}
	INFO("Pixel Format: " << format
		 << "\nDouble buffer: " << ((descriptor.dwFlags & PFD_DOUBLEBUFFER) ? "Yes" : "No")
		 << "\nSamples: " << samples);
}

void windows_gl_context::create_dummy(HDC device_context_handle) {
	this->device_context_handle = device_context_handle;
	set_pixel_format(device_context_handle);
	MESSAGE("Creating dummy context");
	gl_context = wglCreateContext(device_context_handle);
	if (!gl_context) {
		CRITICAL("Failed to create dummy context");
		return;
	}
	make_current();
	initialize_glew();
}

void windows_gl_context::create_with_attributes(HDC device_context_handle, int samples) {
	this->device_context_handle = device_context_handle;
	set_pixel_format_arb(device_context_handle, samples);
	const int attributes[] = {
		WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
		WGL_CONTEXT_MINOR_VERSION_ARB, 6,
		0
	};
	MESSAGE("Creating context with attributes");
	gl_context = wglCreateContextAttribsARB(device_context_handle, nullptr, attributes);
	is_arb_context = (gl_context != nullptr);
	if (!is_arb_context) {
		WARNING("Failed to create context. OpenGL " << attributes[1] << "." << attributes[3] << " not supported");
		MESSAGE("Creating fallback context");
		gl_context = wglCreateContext(device_context_handle);
		if (!gl_context) {
			CRITICAL("Failed to create fallback context");
			return;
		}
	}
	make_current();
	initialize_gl();
}

void windows_gl_context::initialize_glew() {
	auto error = glewInit();
	ASSERT(error == GLEW_OK);
	if (error != GLEW_OK) {
		CRITICAL("Failed to initialize GLEW");
	}
}

void windows_gl_context::initialize_gl() {
	CHECK_GL_ERROR(glEnable(GL_DEPTH_TEST));
	CHECK_GL_ERROR(glDepthFunc(GL_LEQUAL));
	CHECK_GL_ERROR(glEnable(GL_BLEND));
	if (is_arb_context) {
		CHECK_GL_ERROR(glEnable(GL_MULTISAMPLE));
	} else {
		WARNING("Not an ARB context. Multisampling disabled");
	}
	// https://www.opengl.org/sdk/docs/man/html/glBlendFunc.xhtml
	CHECK_GL_ERROR(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
}

HGLRC windows_gl_context::handle() const {
	return gl_context;
}

bool windows_gl_context::exists() const {
	return gl_context != nullptr;
}

bool windows_gl_context::is_current() const {
	return current_context_handle() == gl_context;
}

void windows_gl_context::make_current() {
	wglMakeCurrent(device_context_handle, gl_context);
	initialize_glew();
}

void windows_gl_context::set_viewport(int x, int y, int width, int height) {
	glViewport(x, y, width, height);
}

void windows_gl_context::set_scissor(int x, int y, int width, int height) {
	glScissor(x, y, width, height);
}

void windows_gl_context::set_clear_color(const vector3f& color) {
	glClearColor(color.x, color.y, color.z, 1.0f);
}

bool windows_gl_context::set_swap_interval(int interval) {
	auto status = wglSwapIntervalEXT(interval);
	if (status) {
		MESSAGE("Set swap interval to " << interval);
	} else {
		WARNING("Failed to set swap interval to " << interval << ". Error: " << GetLastError());
	}
	return status;
}

bool windows_gl_context::set_swap_interval(swap_interval interval) {
	return [&] {
		switch (interval) {
		case swap_interval::late: return set_swap_interval(-1);
		case swap_interval::immediate: return set_swap_interval(0);
		case swap_interval::sync: return set_swap_interval(1);
		default: return false;
		}
	}();
}

void windows_gl_context::clear() {
	// todo: can check is_current() and change context, but does that cause more harm than good?
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void windows_gl_context::destroy() {
	if (exists()) {
		if (is_current()) {
			wglMakeCurrent(nullptr, nullptr);
		}
		wglDeleteContext(gl_context);
	}
}

void windows_gl_context::log_renderer_info() const {
	INFO("[b]-- Windows OpenGL --[/b]"
		 << "\n[b]Version:[/b] " << glGetString(GL_VERSION)
		 << "\n[b]Vendor:[/b] " << glGetString(GL_VENDOR)
		 << "\n[b]Renderer:[/b] " << glGetString(GL_RENDERER)
		 << "\n[b]Shading Language Version:[/b] " << glGetString(GL_SHADING_LANGUAGE_VERSION));
}

HGLRC windows_gl_context::current_context_handle() {
	return wglGetCurrentContext();
}

}

}

#endif
