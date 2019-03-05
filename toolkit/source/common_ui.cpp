#include "common_ui.hpp"

void imgui_draw_grid(ImDrawList* draw_list, no::vector2f offset) {
	const float grid_size = 32.0f;
	const ImColor line_color{ 200, 200, 200, 40 };
	no::vector2f win_pos = ImGui::GetCursorScreenPos();
	no::vector2f canvas_size = ImGui::GetWindowSize();
	for (float x = std::fmodf(offset.x, grid_size); x < canvas_size.x; x += grid_size) {
		no::vector2f pos1 = { x + win_pos.x, win_pos.y };
		no::vector2f pos2 = { x + win_pos.x, canvas_size.y + win_pos.y };
		draw_list->AddLine(pos1, pos2, line_color);
	}
	for (float y = std::fmodf(offset.y, grid_size); y < canvas_size.y; y += grid_size) {
		no::vector2f pos1 = { win_pos.x, y + win_pos.y };
		no::vector2f pos2 = { canvas_size.x + win_pos.x, y + win_pos.y };
		draw_list->AddLine(pos1, pos2, line_color);
	}
}
