#pragma once

#include "debug.hpp"

#define PLATFORM_WINDOWS (_WIN32 || _WIN64)

#define ENABLE_GL         1
#define ENABLE_WINDOWS_GL 1

namespace no {

#if ENABLE_GL

#define VERBOSE_GL_LOGGING 0

#if DEBUG_ENABLED
# if VERBOSE_GL_LOGGING
#  define LOG_VERBOSE_GL(GL_CALL) INFO(#GL_CALL);
# else
#  define LOG_VERBOSE_GL(GL_CALL) 
# endif
# define CHECK_GL_ERROR(GL_CALL) \
	GL_CALL; \
	LOG_VERBOSE_GL(GL_CALL) \
	{ \
		auto gl_error = glGetError(); \
		if (gl_error != GL_NO_ERROR) { \
			CRITICAL(#GL_CALL << "\n" << gluErrorString(gl_error)); \
			ASSERT(gl_error == GL_NO_ERROR); \
		} \
	}
#else
# define CHECK_GL_ERROR(GL_CALL) GL_CALL;
#endif

#endif

namespace platform {

#if PLATFORM_WINDOWS

class windows_window;
using platform_window = windows_window;

#if ENABLE_WINDOWS_GL
class windows_gl_context;
using platform_render_context = windows_gl_context;
#endif

#endif

long long performance_frequency();
long long performance_counter();

std::string environment_variable(const std::string& name);

// will block until a file is picked or window is closed
// todo: does this work similar on other platforms? might be a bad abstraction
std::string open_file_browse_window();

std::vector<std::string> command_line_arguments();
void relaunch();

}

}
