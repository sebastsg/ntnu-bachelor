#include "windows_window.hpp"

#if PLATFORM_WINDOWS

#include <Windows.h>
#include <windowsx.h>

#include "debug.hpp"
#include "window.hpp"
#include "input.hpp"

static no::vector2i current_mouse_position;

LRESULT WINAPI process_window_messages(HWND window_handle, UINT message, WPARAM w_param, LPARAM l_param) {
	no::window* active_window = (no::window*)GetWindowLongPtr(window_handle, 0);
	if (!active_window) {
		return DefWindowProc(window_handle, message, w_param, l_param);
	}
	auto& keyboard = active_window->keyboard;
	auto& mouse = active_window->mouse;
	switch (message) {
	case WM_PAINT:
		return DefWindowProc(window_handle, message, w_param, l_param);
	case WM_INPUT:
		return 0;
	case WM_MOUSEMOVE:
	{
		int new_x = GET_X_LPARAM(l_param);
		int new_y = GET_Y_LPARAM(l_param);
		int relative_x = new_x - current_mouse_position.x;
		int relative_y = new_y - current_mouse_position.y;
		if (relative_x == 0 && relative_y == 0) {
			return 0;
		}
		current_mouse_position = { new_x, new_y };
		mouse.move.emit({ { relative_x, relative_y }, { new_x, new_y } });
		return 0;
	}
	case WM_LBUTTONDOWN:
		mouse.press.emit({ no::mouse::button::left });
		return 0;
	case WM_LBUTTONUP:
		mouse.release.emit({ no::mouse::button::left });
		return 0;
	case WM_LBUTTONDBLCLK:
		mouse.double_click.emit({ no::mouse::button::left });
		return 0;
	case WM_RBUTTONDOWN:
		mouse.press.emit({ no::mouse::button::right });
		return 0;
	case WM_RBUTTONUP:
		mouse.release.emit({ no::mouse::button::right });
		return 0;
	case WM_RBUTTONDBLCLK:
		mouse.double_click.emit({ no::mouse::button::right });
		return 0;
	case WM_MBUTTONDOWN:
		mouse.press.emit({ no::mouse::button::middle });
		return 0;
	case WM_MBUTTONUP:
		mouse.release.emit({ no::mouse::button::middle });
		return 0;
	case WM_MBUTTONDBLCLK:
		mouse.double_click.emit({ no::mouse::button::middle });
		return 0;
	case WM_MOUSEWHEEL:
		// TODO: Maybe this won't work for mouse wheels without notches.
		mouse.scroll.emit({ GET_WHEEL_DELTA_WPARAM(w_param) / WHEEL_DELTA });
		return 0;
	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
	{
		int repeat = l_param >> 30;
		keyboard.repeated_press.emit({ (no::key)w_param, repeat });
		if (repeat == 0) {
			keyboard.press.emit({ (no::key)w_param });
		}
		return 0;
	}
	case WM_SYSKEYUP:
	case WM_KEYUP:
		keyboard.release.emit({ (no::key)w_param });
		return 0;
	case WM_CHAR:
		keyboard.input.emit({ w_param });
		return 0;
	case WM_SIZE:
		active_window->set_viewport(0, 0, LOWORD(l_param), HIWORD(l_param));
		active_window->resize.emit({ { LOWORD(l_param), HIWORD(l_param) } });
		return 0;
	case WM_SETCURSOR:
		if (LOWORD(l_param) == HTCLIENT) {
			mouse.cursor.emit();
			return 1;
		}
		return 0;
	case WM_ERASEBKGND:
		return 1;
	case WM_CLOSE:
		active_window->close.emit();
		return 0;
	default:
		return DefWindowProc(window_handle, message, w_param, l_param);
	}
}

namespace no {

namespace platform {

static bool create_window_class(WNDPROC procedure, const std::string& name) {
	MESSAGE("Registering window class " << name);
	WNDCLASS window_class = {};
	window_class.style = CS_OWNDC | CS_DBLCLKS;
	window_class.lpfnWndProc = procedure;
	window_class.cbClsExtra = 0;
	window_class.cbWndExtra = sizeof(LONG_PTR);
	window_class.hInstance = windows::current_instance();
	window_class.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	window_class.hCursor = LoadCursor(NULL, IDC_ARROW);
	window_class.hbrBackground = nullptr;
	window_class.lpszMenuName = nullptr;
	window_class.lpszClassName = name.c_str();
	auto result = RegisterClass(&window_class);
	if (!result) {
		CRITICAL("Failed to register window class");
	}
	return result;
}

static vector2i calculate_actual_window_size(int width, int height, DWORD style) {
	RECT area = { 0, 0, width, height };
	AdjustWindowRect(&area, style, false);
	return { area.right - area.left, area.bottom - area.top };
}

static HWND create_window(const std::string& title, const std::string& class_name, int width, int height, bool is_maximized) {
	MESSAGE("Creating window " <<  title << " of class " << class_name);
	DWORD style = WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
	int x = CW_USEDEFAULT;
	int y = CW_USEDEFAULT;
	auto size = calculate_actual_window_size(width, height, style);
	if (is_maximized) {
		RECT work_area;
		SystemParametersInfo(SPI_GETWORKAREA, 0, &work_area, 0);
		x = work_area.left;
		y = work_area.top;
		size.x = work_area.right;
		size.y = work_area.bottom;
	}
	auto instance = windows::current_instance();
	return CreateWindow(class_name.c_str(), title.c_str(), style, x, y, size.x, size.y, nullptr, nullptr, instance, nullptr);
}

static void destroy_window(HWND window_handle, HDC device_context_handle, windows_gl_context& context) {
	if (!window_handle) {
		return;
	}
	MESSAGE("Destroying window");
	ReleaseDC(window_handle, device_context_handle);
	context.destroy();
	DestroyWindow(window_handle);
}

windows_window::windows_window(window* base_window, const std::string& title, int width, int height, int samples, bool is_maximized) {
	create_window_class(DefWindowProc, "Dummy");
	HWND dummy_window_handle = create_window("Dummy", "Dummy", 4, 4, false);
	if (!dummy_window_handle) {
		CRITICAL("Failed to create dummy window");
		return;
	}
	HDC dummy_device_context_handle = GetDC(dummy_window_handle);
	context.create_dummy(dummy_device_context_handle);
	destroy_window(dummy_window_handle, dummy_device_context_handle, context);

	create_window_class(process_window_messages, "Main");
	window_handle = create_window(title, "Main", width, height, is_maximized);
	if (!window_handle) {
		CRITICAL("Failed to create window");
		return;
	}
	set_base_window(base_window);
	device_context_handle = GetDC(window_handle);
	context.create_with_attributes(device_context_handle, samples);
	context.log_renderer_info();

	int show_command = windows::show_command();
	if ((show_command == SW_SHOWDEFAULT || show_command == SW_SHOWNORMAL) && is_maximized) {
		show_command = SW_MAXIMIZE;
	}
	ShowWindow(window_handle, show_command);

	base_window->set_clear_color(0.1f);
}

windows_window::~windows_window() {
	destroy_window(window_handle, device_context_handle, context);
}

void windows_window::poll() {
	MSG message;
	while (PeekMessage(&message, window_handle, 0, 0, PM_REMOVE)) {
		TranslateMessage(&message);
		DispatchMessage(&message);
	}
}

void windows_window::set_base_window(window* new_window) {
	base_window = new_window;
	SetWindowLongPtr(window_handle, 0, reinterpret_cast<LONG_PTR>(base_window));
}

bool windows_window::is_open() const {
	return window_handle != nullptr;
}

vector2i windows_window::position() const {
	RECT rectangle;
	GetWindowRect(window_handle, &rectangle);
	return { (int)rectangle.left, (int)rectangle.top };
}

vector2i windows_window::size() const {
	RECT rectangle;
	GetClientRect(window_handle, &rectangle);
	return { (int)rectangle.right, (int)rectangle.bottom };
}

std::string windows_window::title() const {
	// int size = GetWindowTextLength(windowHandle);
	CHAR buffer[128] = {};
	GetWindowText(window_handle, buffer, 127);
	return buffer;
}

void windows_window::set_size(const vector2i& size) {
	RECT rectangle = { 0, 0, size.x, size.y };
	long style = GetWindowLong(window_handle, GWL_STYLE);
	AdjustWindowRect(&rectangle, style, false);
	rectangle.right -= rectangle.left;
	rectangle.bottom -= rectangle.top;
	SetWindowPos(window_handle, NULL, 0, 0, rectangle.right, rectangle.bottom, SWP_NOMOVE | SWP_NOZORDER);
}

void windows_window::set_title(const std::string& title) {
	SetWindowText(window_handle, title.c_str());
}

void windows_window::set_viewport(int x, int y, int width, int height) {
	context.set_viewport(x, y, width, height);
}

void windows_window::set_scissor(int x, int y, int width, int height) {
	context.set_scissor(x, y, width, height);
}

void windows_window::set_clear_color(const vector3f& color) {
	context.set_clear_color(color);
}

bool windows_window::set_swap_interval(swap_interval interval) {
	return context.set_swap_interval(interval);
}

void windows_window::clear() {
	context.clear();
}

void windows_window::swap() {
	BOOL result = SwapBuffers(device_context_handle);
	if (!result) {
		DWORD error = GetLastError();
		WARNING_LIMIT("HDC: " << device_context_handle << "\nHWND: " << window_handle << "\nError: " << error, 10);
	}
}

HWND windows_window::handle() const {
	return window_handle;
}

}

}

#endif
