#include "platform.hpp"
#include "io.hpp"
#include "debug.hpp"
#include "loop.hpp"

#if PLATFORM_WINDOWS

#include <Windows.h>

#include "windows_platform.hpp"

// these are defined by Windows when using vc++ runtime
extern int __argc;
extern char** __argv;

namespace no {

namespace platform {

namespace windows {

static HINSTANCE current_instance_arg = nullptr;
static int show_command_arg = 0;

HINSTANCE current_instance() {
	return current_instance_arg;
}

int show_command() {
	return show_command_arg;
}

}

long long performance_frequency() {
	LARGE_INTEGER frequency;
	QueryPerformanceFrequency(&frequency);
	return frequency.QuadPart;
}

long long performance_counter() {
	LARGE_INTEGER counter;
	QueryPerformanceCounter(&counter);
	return counter.QuadPart;
}

std::string environment_variable(const std::string& name) {
	char buffer[2048];
	GetEnvironmentVariableA(name.c_str(), buffer, 2048);
	return buffer;
}

std::string open_file_browse_window() {
	char file[MAX_PATH];
	char file_title[MAX_PATH];
	char template_name[MAX_PATH];
	OPENFILENAME data = {};
	data.lStructSize = sizeof(data);
	data.hwndOwner = nullptr;
	data.lpstrDefExt = nullptr;
	data.lpstrCustomFilter = nullptr;
	data.lpstrFile = file;
	data.lpstrFile[0] = '\0';
	data.nMaxFile = MAX_PATH;
	data.lpstrFileTitle = file_title;
	data.lpstrFileTitle[0] = '\0';
	data.lpTemplateName = template_name;
	data.nMaxFileTitle = 0;
	data.lpstrFilter = "All\0*.*\0";
	data.nFilterIndex = 0;
	data.lpstrInitialDir = nullptr;
	data.lpstrTitle = nullptr;
	data.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
	GetOpenFileName(&data);
	return file;
}

std::vector<std::string> command_line_arguments() {
	std::vector<std::string> args;
	for (int i = 0; i < __argc; i++) {
		args.push_back(__argv[i]);
	}
	return args;
}

void relaunch() {
	STARTUPINFO startup{};
	startup.cb = sizeof(startup);
	PROCESS_INFORMATION process{};
	auto success = CreateProcess(__argv[0], nullptr, nullptr, nullptr, FALSE, 0, nullptr, nullptr, &startup, &process);
	if (success) {
		// note: doesn't close process
		CloseHandle(process.hProcess);
		CloseHandle(process.hThread);
	} else {
		WARNING("Failed to start " << __argv[0] << ". Error: " << GetLastError());
	}
	internal::destroy_main_loop();
	std::exit(0);
}

}

}

int WINAPI WinMain(HINSTANCE current_instance, HINSTANCE previous_instance, LPSTR command_line, int show_command) {
	no::platform::windows::current_instance_arg = current_instance;
	no::platform::windows::show_command_arg = show_command;
	CoInitialize(nullptr);
	int result = no::internal::run_main_loop();
	CoUninitialize();
	return result;
}

#endif
