#include <glew/glew.h>

#include <Windows.h>
#include <tchar.h>

#include "window.hpp"
#include "windows_window.hpp"
#include "assets.hpp"
#include "camera.hpp"
#include "surface.hpp"

#include "GLM/gtc/matrix_transform.hpp"
#include "GLM/gtc/type_ptr.hpp"

#include "imgui.h"
#include "imgui_platform.h"

namespace no {

namespace imgui {

static struct {
	window* window = nullptr;
	INT64 time = 0;
	INT64 ticks_per_second = 0;
	ImGuiMouseCursor last_mouse_cursor = ImGuiMouseCursor_COUNT;
	int shader_id = -1;
	int font_texture_id = -1;
	int keyboard_repeated_press_id = -1;
	int keyboard_release_id = -1;
	int keybord_input_id = -1;
	int mouse_scroll_id = -1;
	int mouse_cursor_id = -1;
	int mouse_press_id = -1;
	int mouse_double_click_id = -1;
	int mouse_release_id = -1;
} data;

struct imgui_vertex {
	static constexpr vertex_attribute_specification attributes[] = { 2, 2, { attribute_component::is_byte, 4, true } };
	vector2f position;
	vector2f tex_coords;
	uint32_t color = 0;
};

static void update_cursor_icon() {
	auto& io = ImGui::GetIO();
	if (io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange) {
		return;
	}
	ImGuiMouseCursor cursor = ImGui::GetMouseCursor();
	if (cursor == ImGuiMouseCursor_None || io.MouseDrawCursor) {
		SetCursor(nullptr);
		return;
	}
	SetCursor(LoadCursor(nullptr, [cursor] {
		switch (cursor) {
		case ImGuiMouseCursor_Arrow: return IDC_ARROW;
		case ImGuiMouseCursor_TextInput: return IDC_IBEAM;
		case ImGuiMouseCursor_ResizeAll: return IDC_SIZEALL;
		case ImGuiMouseCursor_ResizeEW: return IDC_SIZEWE;
		case ImGuiMouseCursor_ResizeNS: return IDC_SIZENS;
		case ImGuiMouseCursor_ResizeNESW: return IDC_SIZENESW;
		case ImGuiMouseCursor_ResizeNWSE: return IDC_SIZENWSE;
		case ImGuiMouseCursor_Hand: return IDC_HAND;
		default: return IDC_ARROW;
		}
	}()));
}

static void update_mouse_position() {
	auto& io = ImGui::GetIO();
	if (io.WantSetMousePos) {
		POINT pos = { (int)io.MousePos.x, (int)io.MousePos.y };
		ClientToScreen(data.window->platform_window()->handle(), &pos);
		SetCursorPos(pos.x, pos.y);
	}
	io.MousePos = { -FLT_MAX, -FLT_MAX };
	POINT pos;
	if (GetActiveWindow() == data.window->platform_window()->handle() && GetCursorPos(&pos)) {
		if (ScreenToClient(data.window->platform_window()->handle(), &pos)) {
			io.MousePos = { (float)pos.x, (float)pos.y };
		}
	}
}

static void set_mouse_down(mouse::button button, bool is_down) {
	auto& io = ImGui::GetIO();
	switch (button) {
	case mouse::button::left:
		io.MouseDown[0] = is_down;
		break;
	case mouse::button::right:
		io.MouseDown[1] = is_down;
		break;
	case mouse::button::middle:
		io.MouseDown[2] = is_down;
		break;
	default:
		break;
	}
}

void create(window& window) {
	ASSERT(!data.window);
	ImGui::CreateContext();
	data.window = &window;
	QueryPerformanceFrequency((LARGE_INTEGER*)&data.ticks_per_second);
	QueryPerformanceCounter((LARGE_INTEGER*)&data.time);
	auto& io = ImGui::GetIO();
	io.BackendFlags = ImGuiBackendFlags_HasMouseCursors | ImGuiBackendFlags_HasSetMousePos;
	io.BackendPlatformName = "win32";
	io.ImeWindowHandle = window.platform_window()->handle();
	io.KeyMap[ImGuiKey_Tab] = VK_TAB;
	io.KeyMap[ImGuiKey_LeftArrow] = VK_LEFT;
	io.KeyMap[ImGuiKey_RightArrow] = VK_RIGHT;
	io.KeyMap[ImGuiKey_UpArrow] = VK_UP;
	io.KeyMap[ImGuiKey_DownArrow] = VK_DOWN;
	io.KeyMap[ImGuiKey_PageUp] = VK_PRIOR;
	io.KeyMap[ImGuiKey_PageDown] = VK_NEXT;
	io.KeyMap[ImGuiKey_Home] = VK_HOME;
	io.KeyMap[ImGuiKey_End] = VK_END;
	io.KeyMap[ImGuiKey_Insert] = VK_INSERT;
	io.KeyMap[ImGuiKey_Delete] = VK_DELETE;
	io.KeyMap[ImGuiKey_Backspace] = VK_BACK;
	io.KeyMap[ImGuiKey_Space] = VK_SPACE;
	io.KeyMap[ImGuiKey_Enter] = VK_RETURN;
	io.KeyMap[ImGuiKey_Escape] = VK_ESCAPE;
	io.KeyMap[ImGuiKey_A] = 'A';
	io.KeyMap[ImGuiKey_C] = 'C';
	io.KeyMap[ImGuiKey_V] = 'V';
	io.KeyMap[ImGuiKey_X] = 'X';
	io.KeyMap[ImGuiKey_Y] = 'Y';
	io.KeyMap[ImGuiKey_Z] = 'Z';
	data.keyboard_repeated_press_id = window.keyboard.repeated_press.listen([&](const keyboard::repeated_press_message& event) {
		if ((int)event.key < 256) {
			io.KeysDown[(int)event.key] = true;
		}
	});
	data.keyboard_release_id = window.keyboard.release.listen([&](const keyboard::release_message& event) {
		if ((int)event.key < 256) {
			io.KeysDown[(int)event.key] = false;
		}
	});
	data.keybord_input_id = window.keyboard.input.listen([&](const keyboard::input_message& event) {
		if (event.character > 0 && event.character < 0x10000) {
			io.AddInputCharacter(event.character);
		}
	});
	data.mouse_scroll_id = window.mouse.scroll.listen([&](const mouse::scroll_message& event) {
		io.MouseWheel += event.steps;
	});
	data.mouse_cursor_id = window.mouse.icon.listen([] {
		update_cursor_icon();
	});
	data.mouse_press_id = window.mouse.press.listen([&](const mouse::press_message& event) {
		if (!ImGui::IsAnyMouseDown() && !GetCapture()) {
			SetCapture(data.window->platform_window()->handle());
		}
		set_mouse_down(event.button, true);
	});
	data.mouse_double_click_id = window.mouse.double_click.listen([&](const mouse::double_click_message& event) {
		if (!ImGui::IsAnyMouseDown() && !GetCapture()) {
			SetCapture(data.window->platform_window()->handle());
		}
		set_mouse_down(event.button, true);
	});
	data.mouse_release_id = window.mouse.release.listen([&](const mouse::release_message& event) {
		set_mouse_down(event.button, false);
		if (!ImGui::IsAnyMouseDown() && GetCapture() == data.window->platform_window()->handle()) {
			ReleaseCapture();
		}
	});
	io.BackendRendererName = "opengl-no";

	ImGui::StyleColorsDark();

	data.shader_id = create_shader(asset_path("shaders/imgui"));

	unsigned char* pixels = nullptr;
	int width = 0;
	int height = 0;
	io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

	data.font_texture_id = create_texture();
	bind_texture(data.font_texture_id);
	load_texture(data.font_texture_id, { (uint32_t*)pixels, width, height, pixel_format::rgba, surface::construct_by::copy });

	io.Fonts->TexID = (ImTextureID)data.font_texture_id;
}

void destroy() {
	data.window->keyboard.repeated_press.ignore(data.keyboard_repeated_press_id);
	data.window->keyboard.release.ignore(data.keyboard_release_id);
	data.window->keyboard.input.ignore(data.keybord_input_id);
	data.window->mouse.scroll.ignore(data.mouse_scroll_id);
	data.window->mouse.icon.ignore(data.mouse_cursor_id);
	data.window->mouse.press.ignore(data.mouse_press_id);
	data.window->mouse.double_click.ignore(data.mouse_double_click_id);
	data.window->mouse.release.ignore(data.mouse_release_id);
	delete_shader(data.shader_id);
	delete_texture(data.font_texture_id);
	data.shader_id = -1;
	data.font_texture_id = -1;
	ImGui::GetIO().Fonts->TexID = 0;
	data.window = nullptr;
}

void start_frame() {
	auto& io = ImGui::GetIO();

	RECT rect;
	GetClientRect(data.window->platform_window()->handle(), &rect);
	io.DisplaySize = { (float)(rect.right - rect.left), (float)(rect.bottom - rect.top) };

	INT64 current_time;
	QueryPerformanceCounter((LARGE_INTEGER*)&current_time);
	io.DeltaTime = (float)(current_time - data.time) / data.ticks_per_second;
	data.time = current_time;

	io.KeyCtrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
	io.KeyShift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
	io.KeyAlt = (GetKeyState(VK_MENU) & 0x8000) != 0;
	io.KeySuper = false;

	update_mouse_position();

	ImGuiMouseCursor mouse_cursor = io.MouseDrawCursor ? ImGuiMouseCursor_None : ImGui::GetMouseCursor();
	if (data.last_mouse_cursor != mouse_cursor) {
		data.last_mouse_cursor = mouse_cursor;
		update_cursor_icon();
	}

	ImGui::NewFrame();
}

void end_frame() {
	ImGui::Render();
}

void draw() {
	auto draw_data = ImGui::GetDrawData();
	if (!draw_data) {
		return;
	}
	auto& io = ImGui::GetIO();
	int fb_width = (int)(draw_data->DisplaySize.x * io.DisplayFramebufferScale.x);
	int fb_height = (int)(draw_data->DisplaySize.y * io.DisplayFramebufferScale.y);
	if (fb_width <= 0 || fb_height <= 0) {
		return;
	}
	draw_data->ScaleClipRects(io.DisplayFramebufferScale);

	vector2f pos = {
		draw_data->DisplayPos.x,
		draw_data->DisplayPos.y
	};
	bind_shader(data.shader_id);

	ortho_camera camera;
	camera.transform.scale = { draw_data->DisplaySize.x, draw_data->DisplaySize.y };
	set_shader_view_projection(camera);

	vertex_array<imgui_vertex> vertex_array;
	for (int list_index = 0; list_index < draw_data->CmdListsCount; list_index++) {
		auto cmd_list = draw_data->CmdLists[list_index];
		auto vertex_data = (uint8_t*)cmd_list->VtxBuffer.Data;
		auto index_data = (uint8_t*)cmd_list->IdxBuffer.Data;
		vertex_array.set(vertex_data, cmd_list->VtxBuffer.Size, index_data, cmd_list->IdxBuffer.Size);
		size_t offset = 0;
		for (int buffer_index = 0; buffer_index < cmd_list->CmdBuffer.Size; buffer_index++) {
			auto buffer = &cmd_list->CmdBuffer[buffer_index];
			const size_t current_offset = offset;
			offset += buffer->ElemCount;
			if (buffer->UserCallback) {
				buffer->UserCallback(cmd_list, buffer);
				continue;
			}
			vector4i clip_rect = {
				(int)buffer->ClipRect.x - (int)pos.x,
				(int)buffer->ClipRect.y - (int)pos.y,
				(int)buffer->ClipRect.z - (int)pos.x,
				(int)buffer->ClipRect.w - (int)pos.y
			};
			if (clip_rect.x >= fb_width || clip_rect.y >= fb_height || clip_rect.z < 0 || clip_rect.w < 0) {
				continue;
			}
			bind_texture((int)buffer->TextureId);
			vertex_array.draw(current_offset, buffer->ElemCount);
		}
	}
}

}

}
