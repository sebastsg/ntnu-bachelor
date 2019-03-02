#include "dialogue_editor.hpp"
#include "window.hpp"
#include "assets.hpp"
#include "platform.hpp"
#include "surface.hpp"

#include "imgui/imgui.h"
#include "imgui/imgui_platform.h"

#include <filesystem>

static no::vector2f item_uv1(no::vector2f uv, int texture) {
	return uv / no::texture_size(texture).to<float>();
}

static no::vector2f item_uv2(no::vector2f uv, int texture) {
	return (uv + 32.0f) / no::texture_size(texture).to<float>();
}

dialogue_editor_state::dialogue_editor_state() {
	no::imgui::create(window());
	ui_texture = no::create_texture(no::surface(no::asset_path("sprites/ui.png")));
	blank_texture = no::create_texture(no::surface{ 2, 2, no::pixel_format::rgba, 0xFFFFFFFF });
	if (dialogue_meta().count() > 0) {
		load_dialogue(0);
	} else {
		create_new_dialogue();
	}
}

dialogue_editor_state::~dialogue_editor_state() {
	no::delete_texture(ui_texture);
	no::delete_texture(blank_texture);
	no::imgui::destroy();
}

void dialogue_editor_state::destroy_current_dialogue() {
	dialogue = {};
	selected_dialogue_index = -1;
	node_index_link_from = -1;
	node_link_from_out = -1;
	node_selected = -1;
	scrolling = 0;
}

void dialogue_editor_state::create_new_dialogue() {
	destroy_current_dialogue();
	dialogue.id = dialogue_meta().add("New dialogue").id;
	selected_dialogue_index = dialogue_meta().find_index_by_id(dialogue.id);
}

void dialogue_editor_state::load_dialogue(int index) {
	destroy_current_dialogue();
	auto item = dialogue_meta().find_by_index(index);
	if (item) {
		selected_dialogue_index = index;
		dialogue.load(item->id);
	}
}

void dialogue_editor_state::save_dialogue() {
	dialogue.save();
	dialogue_meta().save();
	dirty = false;
}

void dialogue_editor_state::update() {
	no::imgui::start_frame();
	menu_bar_state::update();
	ImGuiWindowFlags imgui_flags
		= ImGuiWindowFlags_NoMove
		| ImGuiWindowFlags_NoResize
		| ImGuiWindowFlags_NoCollapse
		| ImGuiWindowFlags_NoTitleBar;
	ImGui::SetNextWindowPos({ 0.0f, 20.0f }, ImGuiSetCond_Once);
	ImGui::SetNextWindowSize(window().size().to<float>(), ImGuiSetCond_Always);
	ImGui::Begin("##DialogueEditor", nullptr, imgui_flags);

	ImGui::BeginChild("##NodeList", { 192, 0 });
	update_header();
	update_dialogue_list();
	update_selected_dialogue();
	ImGui::EndChild();

	ImGui::SameLine(); // important for scrolling region to be placed correctly

	ImGui::BeginGroup();

	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 1, 1 });
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0, 0 });
	ImGui::PushStyleColor(ImGuiCol_ChildWindowBg, ImColor(60, 60, 70, 200).Value);
	ImGui::BeginChild("##NodeGrid", { 0, 0 }, true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove);
	ImGui::PushItemWidth(120.0f);

	no::vector2f offset = ImGui::GetCursorScreenPos();
	offset -= scrolling;
	ImDrawList* draw_list = ImGui::GetWindowDrawList();
	draw_list->ChannelsSplit(2);
	update_grid(draw_list, offset);
	draw_list->ChannelsSetCurrent(0);
	update_node_links(draw_list, offset);

	can_show_context_menu = true;
	open_context_menu = false;
	update_nodes(draw_list, offset);
	draw_list->ChannelsMerge();

	if (can_show_context_menu) {
		if (!ImGui::IsAnyItemHovered() && ImGui::IsMouseHoveringWindow() && ImGui::IsMouseClicked(1)) {
			node_selected = -1;
			node_index_hovered = -1;
			open_context_menu = true;
		}
		if (open_context_menu) {
			ImGui::OpenPopup("GridContextMenu");
			if (node_index_hovered != -1) {
				node_selected = node_index_hovered;
			}
		}
	}

	update_context_menu(offset);
	update_scrolling();

	ImGui::PopItemWidth();
	ImGui::EndChild();
	ImGui::PopStyleColor();
	ImGui::PopStyleVar(2);
	ImGui::EndGroup();

	ImGui::End();
	no::imgui::end_frame();
}

void dialogue_editor_state::draw() {
	no::imgui::draw();
}

void dialogue_editor_state::update_header() {
	ImGui::Text(CSTRING("Saved: " << (dirty ? "No " : "Yes")));
	ImGui::SameLine();
	no::vector4f saved{ 0.2f, 1.0f, 0.4f, 0.7f };
	no::vector4f unsaved{ 0.8f, 0.8f, 0.1f, 0.7f };
	no::vector4f status{ dirty ? unsaved : saved };
	no::vector4f border{ status + no::vector4f{ 0.2f, 0.2f, 0.2f, 0.0f } };
	ImGui::Image((ImTextureID)blank_texture, { 10.0f }, { 0.0f }, { 1.0f }, status, border);
}

void dialogue_editor_state::update_dialogue_list() {
	int new_dialogue_index = selected_dialogue_index;
	ImGui::PushItemWidth(-1);
	ImGui::ListBox("##DialogueMetaList", &new_dialogue_index, [](void* data, int i, const char** out_text) -> bool {
		auto item = dialogue_meta().find_by_index(i);
		if (!item) {
			return false;
		}
		*out_text = &item->name[0];
		return true;
	}, this, dialogue_meta().count(), 20);
	ImGui::PopItemWidth();
	if (new_dialogue_index != selected_dialogue_index) {
		save_dialogue();
		load_dialogue(new_dialogue_index);
	}
	if (ImGui::Button("Create new dialogue")) {
		create_new_dialogue();
	}
}

void dialogue_editor_state::update_selected_dialogue() {
	auto item = dialogue_meta().find_by_index(selected_dialogue_index);
	if (!item) {
		return;
	}
	ImGui::PushID("SelectedDialogue");
	ImGui::Separator();
	ImGui::Text("Name:");
	ImGui::SameLine();
	imgui_input_text<128>("##Name", item->name);
	ImGui::Text("Dialogue ID: %i", item->id);
	ImGui::Text("Nodes: %i", dialogue.nodes.size());
	if (ImGui::Button("Save")) {
		save_dialogue();
	}
	ImGui::Separator();
	ImGui::PopID();
}

void dialogue_editor_state::update_grid(ImDrawList* draw_list, no::vector2f offset) {
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

void dialogue_editor_state::update_node_links(ImDrawList* draw_list, no::vector2f offset) {
	for (auto& i : dialogue.nodes) {
		auto node = i.second;
		for (auto& j : node->out) {
			if (j.node_id == -1) {
				continue;
			}
			auto out_node = dialogue.nodes[j.node_id];
			const no::vector2f size1 = node->transform.scale.xy;
			const no::vector2f size2 = out_node->transform.scale.xy;
			no::vector2f pos1 = node->transform.position.xy;
			no::vector2f pos2 = out_node->transform.position.xy;

			float delta_nx = std::abs((pos2.x + size2.x / 2.0f) - (pos1.x + size1.x / 2.0f));
			float delta_ny = std::abs((pos2.y + size2.y / 2.0f) - (pos1.y + size1.y / 2.0f));
			bool delta_x_greater_than_y = delta_nx > delta_ny;

			bool n1_left_all = false;
			bool n1_left_part = false;
			bool n1_right_all = false;
			bool n1_right_part = false;
			if (pos1.x < pos2.x) {
				n1_left_part = true;
				if (pos1.x + size1.x < pos2.x) {
					n1_left_all = true;
				}
			} else {
				n1_right_part = true;
				if (pos1.x > pos2.x + size2.x) {
					n1_right_all = true;
				}
			}

			bool n1_top_all = false;
			bool n1_top_part = false;
			bool n1_bottom_all = false;
			bool n1_bottom_part = false;
			if (pos1.y < pos2.y) {
				n1_top_part = true;
				if (pos1.y + size1.y < pos2.y) {
					n1_top_all = true;
				}
			} else {
				n1_bottom_part = true;
				if (pos1.y > pos2.y + size2.y) {
					n1_bottom_all = true;
				}
			}

			n1_top_all = n1_top_part;
			n1_bottom_all = n1_bottom_part;

			ImU32 stroke_color = ImColor(255, 255, 255);
			float bez_factor = 50.f;
			float bx1 = bez_factor;
			float bx2 = -bez_factor;
			float by1 = 0.0f;
			float by2 = 0.0f;

			if (n1_top_all) {
				if (delta_x_greater_than_y) {
					if (n1_left_all) {
						pos1.x += size1.x;
						pos1.y += size1.y / 2.0f;
						pos2.y += size2.y / 2.0f;
					} else if (n1_right_all) {
						pos1.y += size1.y / 2.0f;
						pos2.x += size2.x;
						pos2.y += size2.y / 2.0f;
						bx1 = -bez_factor;
						bx2 = bez_factor;
					} else if (n1_left_part || n1_right_part) {
						bx1 = 0.0f;
						bx2 = 0.0f;
						pos1.x += size1.x / 2.0f;
						pos1.y += size1.y;
						pos2.x += size2.x / 2.0f;
						by1 = bez_factor;
						by2 = -bez_factor;
					}
				} else {
					pos1.x += size1.x / 2.0f;
					pos1.y += size1.y;
					pos2.x += size2.x / 2.0f;
					bx1 = 0.0f;
					bx2 = 0.0f;
					by1 = bez_factor;
					by2 = -bez_factor;
				}
			} else if (n1_bottom_all) {
				if (delta_x_greater_than_y) {
					if (n1_left_all) {
						pos1.x += size1.x;
						pos1.y += size1.y / 2.0f;
						pos2.y += size2.y / 2.0f;
					} else if (n1_right_all) {
						pos1.y += size1.y / 2.0f;
						pos2.x += size2.x;
						pos2.y += size2.y / 2.0f;
						bx1 = -bez_factor;
						bx2 = bez_factor;
					} else if (n1_left_part || n1_right_part) {
						bx1 = 0.0f;
						bx2 = 0.0f;
						pos1.x += size1.x / 2.0f;
						pos2.x += size2.x / 2.0f;
						pos2.y += size2.y;
						by1 = -bez_factor;
						by2 = bez_factor;
					}
				} else {
					pos1.x += size1.x / 2.0f;
					pos2.x += size2.x / 2.0f;
					pos2.y += size2.y;
					bx1 = 0.0f;
					bx2 = 0.0f;
					by1 = -bez_factor;
					by2 = bez_factor;
				}
			}

			stroke_color = ImColor(200, 200, 100);
			switch (node->output_type()) {
			case node_output_type::boolean:
				if (j.out_id == 0) {
					stroke_color = ImColor(200, 100, 100);
				} else if (j.out_id == 1) {
					stroke_color = ImColor(100, 200, 100);
				}
				break;
			}
			pos1.x += offset.x;
			pos1.y += offset.y;
			pos2.x += offset.x;
			pos2.y += offset.y;

			const no::vector2f pos1b = pos1 + no::vector2f{ bx1, by1 };
			const no::vector2f pos2b = pos2 + no::vector2f{ bx2, by2 };

			draw_list->PathLineTo(pos1);
			draw_list->PathBezierCurveTo(pos1b, pos2b, pos2);
			std::vector<no::vector2f> bezier_copy;
			for (int k = 0; k < draw_list->_Path.Size; k++) {
				bezier_copy.push_back({ draw_list->_Path[k].x, draw_list->_Path[k].y });
			}
			static float ct = 0.0f;
			draw_list->PathStroke(stroke_color, false, 3.0f + 3.0f * std::abs(ct - 0.5f));
			auto& circles = node_circles[node];
			while (circles.size() <= node->out.size()) {
				circles.push_back({ bezier_copy[0], 1, 0.0f });
			}
			auto& circle = circles[j.out_id];
			ct = circle.t;
			// the curve might have changed:
			if (circle.bezier_index >= (int)bezier_copy.size()) {
				circle.bezier_index = (int)bezier_copy.size() - 1;
				circle.position = bezier_copy[circle.bezier_index - 1];
			}
			const no::vector2f target_pos = bezier_copy[circle.bezier_index];
			no::vector2f start_pos;
			if (circle.bezier_index == 0) {
				start_pos = target_pos;
			} else {
				start_pos = bezier_copy[circle.bezier_index - 1];
			}
			circle.position.x = (1.0f - circle.t) * start_pos.x + circle.t * target_pos.x;
			circle.position.y = (1.0f - circle.t) * start_pos.y + circle.t * target_pos.y;
			circle.t += 0.01f;
			if (circle.t > 1.0f) {
				circle.t = 0.0f;
				circle.bezier_index++;
				if (circle.bezier_index >= (int)bezier_copy.size()) {
					circle.position = bezier_copy[0];
					circle.bezier_index = 1;
				}
			}
			draw_list->AddCircleFilled({ circle.position }, 5.0f + 3.0f * std::abs(circle.t - 0.5f), stroke_color);
		}
	}
}

void dialogue_editor_state::update_nodes(ImDrawList* draw_list, no::vector2f offset) {
	node_index_hovered = -1;
	for (auto& i : dialogue.nodes) {
		ImGui::PushID(i.first);
		auto node = i.second;
		const no::vector2f node_rect_min = offset + node->transform.position.xy;
		const no::vector2f cursor = node_rect_min + 8.0f;
		draw_list->ChannelsSetCurrent(1);
		bool old_any_active = ImGui::IsAnyItemActive();
		ImGui::SetCursorScreenPos(cursor);
		ImGui::BeginGroup();
		switch (node->type()) {
		case node_type::message: update_node_ui(*(message_node*)node); break;
		case node_type::choice: update_node_ui(*(choice_node*)node); break;
		case node_type::has_item_condition: update_node_ui(*(has_item_condition_node*)node); break;
		case node_type::stat_condition: update_node_ui(*(stat_condition_node*)node); break;
		case node_type::inventory_effect: update_node_ui(*(inventory_effect_node*)node); break;
		case node_type::stat_effect: update_node_ui(*(stat_effect_node*)node); break;
		case node_type::warp_effect: update_node_ui(*(warp_effect_node*)node); break;
		case node_type::var_condition: update_node_ui(*(var_condition_node*)node); break;
		case node_type::modify_var: update_node_ui(*(modify_var_node*)node); break;
		case node_type::create_var: update_node_ui(*(create_var_node*)node); break;
		case node_type::var_exists: update_node_ui(*(var_exists_node*)node); break;
		case node_type::delete_var: update_node_ui(*(delete_var_node*)node); break;
		case node_type::random: update_node_ui(*(random_node*)node); break;
		case node_type::random_condition: update_node_ui(*(random_condition_node*)node); break;
		default: break;
		}
		ImGui::EndGroup();

		bool node_widgets_active = (!old_any_active && ImGui::IsAnyItemActive());
		node->transform.scale.x = ImGui::GetItemRectSize().x + 16.0f;
		node->transform.scale.y = ImGui::GetItemRectSize().y + 16.0f;
		ImVec2 node_rect_max = node_rect_min;
		node_rect_max.x += node->transform.scale.x;
		node_rect_max.y += node->transform.scale.y;

		draw_list->ChannelsSetCurrent(0);
		ImGui::SetCursorScreenPos(node_rect_min);
		ImGui::InvisibleButton("node", node->transform.scale.xy);
		if (ImGui::IsItemHovered()) {
			node_index_hovered = i.first;
			open_context_menu |= ImGui::IsMouseClicked(1);
		}
		bool node_moving_active = ImGui::IsItemActive();
		if (node_widgets_active || node_moving_active) {
			node_selected = i.first;
		}
		if (node_moving_active && ImGui::IsMouseDragging(0)) {
			node->transform.position.x += ImGui::GetIO().MouseDelta.x;
			node->transform.position.y += ImGui::GetIO().MouseDelta.y;
		}
		ImU32 node_background_color = ImColor(60, 60, 60);
		if (node_selected == i.first) {
			node_background_color = ImColor(75, 75, 75);
		}
		if (node->id == dialogue.start_node_id) {
			node_background_color = ImColor(75, 90, 75);
		}
		draw_list->AddRectFilled(node_rect_min, node_rect_max, node_background_color, 4.0f);
		draw_list->AddRect(node_rect_min, node_rect_max, ImColor(100, 100, 100), 4.0f);
		ImGui::PopID();
	}
}

void dialogue_editor_state::update_context_menu(no::vector2f offset) {
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
	bool opened = ImGui::BeginPopup("GridContextMenu");
	if (!opened) {
		ImGui::PopStyleVar();
		return;
	}
	abstract_node* node = nullptr;
	if (node_selected != -1) {
		node = dialogue.nodes[node_selected];
	}
	ImVec2 scene_pos = ImGui::GetMousePosOnOpeningCurrentPopup();
	scene_pos.x -= offset.x;
	scene_pos.y -= offset.y;
	if (node) {
		if (node_index_link_from == -1) {
			if (node->output_type() == node_output_type::variable) {
				if (ImGui::MenuItem("Start link")) {
					node_index_link_from = node_selected;
				}
			} else {
				if (ImGui::BeginMenu("Start link")) {
					switch (node->output_type()) {
					case node_output_type::boolean:
						if (ImGui::MenuItem("True")) {
							node_link_from_out = 1;
							node_index_link_from = node_selected;
						}
						if (ImGui::MenuItem("False")) {
							node_link_from_out = 0;
							node_index_link_from = node_selected;
						}
						break;
					case node_output_type::single:
						if (ImGui::MenuItem("Forward")) {
							node_link_from_out = 0;
							node_index_link_from = node_selected;
						}
						break;
					default:
						break;
					}
					ImGui::EndMenu();
				}
			}
		} else {
			auto from_node = dialogue.nodes[node_index_link_from];
			bool can_connect = true;
			if (from_node->id == node->id) {
				can_connect = false;
			}
			if (from_node->type() == node_type::message && node->type() == node_type::message) {
				can_connect = false;
			}
			if (can_connect && ImGui::MenuItem("Connect")) {
				from_node->set_output_node(node_link_from_out, node->id);
				node_index_link_from = -1;
				node_link_from_out = -1;
				dirty = true;
			}
			if (ImGui::MenuItem("Cancel connecting")) {
				node_index_link_from = -1;
				node_link_from_out = -1;
			}
		}
		if (node->type() != node_type::choice && ImGui::MenuItem("Set as starting point")) {
			dialogue.start_node_id = node->id;
			dirty = true;
		}
		if (ImGui::MenuItem("Delete")) {
			for (auto& i : dialogue.nodes) {
				i.second->remove_output_node(node->id);
			}
			delete dialogue.nodes[node_selected];
			dialogue.nodes.erase(node_selected);
			node_selected = -1;
			node_index_link_from = -1;
			node_link_from_out = -1;
			node = nullptr;
			dirty = true;
		}
	} else {
		if (ImGui::BeginMenu("Add node")) {
			if (ImGui::MenuItem("Message")) {
				node = new message_node();
			}
			if (ImGui::MenuItem("Choice")) {
				node = new choice_node();
			}
			if (ImGui::BeginMenu("Conditions")) {
				if (ImGui::MenuItem("Has item")) {
					node = new has_item_condition_node();
				}
				if (ImGui::MenuItem("Check stat")) {
					node = new stat_condition_node();
				}
				if (ImGui::MenuItem("Random")) {
					node = new random_condition_node();
				}
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Effects")) {
				if (ImGui::MenuItem("Give or take item")) {
					node = new inventory_effect_node();
				}
				if (ImGui::MenuItem("Modify stat")) {
					node = new stat_effect_node();
				}
				if (ImGui::MenuItem("Warp")) {
					node = new warp_effect_node();
				}
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Variables")) {
				if (ImGui::MenuItem("Create variable")) {
					node = new create_var_node();
				}
				if (ImGui::MenuItem("Does variable exist")) {
					node = new var_exists_node();
				}
				if (ImGui::MenuItem("Compare variable")) {
					node = new var_condition_node();
				}
				if (ImGui::MenuItem("Modify variable")) {
					node = new modify_var_node();
				}
				if (ImGui::MenuItem("Delete variable")) {
					node = new delete_var_node();
				}
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Special")) {
				if (ImGui::MenuItem("Random")) {
					node = new random_node();
				}
				ImGui::EndMenu();
			}
			ImGui::EndMenu();
		}
		if (node) {
			node->id = dialogue.id_counter;
			dialogue.id_counter++;
			node->transform.position.xy = { scene_pos.x, scene_pos.y };
			dialogue.nodes[node->id] = node;
			dirty = true;
		}
	}
	ImGui::EndPopup();
	ImGui::PopStyleVar();
}

void dialogue_editor_state::update_scrolling() {
	if (ImGui::IsAnyItemActive()) {
		return;
	}
	if (ImGui::IsWindowHovered() && ImGui::IsMouseDragging(2, 0.0f)) {
		scrolling.x -= ImGui::GetIO().MouseDelta.x;
		scrolling.y -= ImGui::GetIO().MouseDelta.y;
	}
	const float scroll_speed = 20.0f;
	if (keyboard().is_key_down(no::key::up)) {
		scrolling.y -= scroll_speed;
	}
	if (keyboard().is_key_down(no::key::left)) {
		scrolling.x -= scroll_speed;
	}
	if (keyboard().is_key_down(no::key::down)) {
		scrolling.y += scroll_speed;
	}
	if (keyboard().is_key_down(no::key::right)) {
		scrolling.x += scroll_speed;
	}
}

void dialogue_editor_state::update_node_ui(message_node& node) {
	ImGui::Text("Message");
	dirty |= imgui_input_text_multiline<1024>("##Message", node.text, { 350.0f, ImGui::GetTextLineHeight() * 8.0f });
}

void dialogue_editor_state::update_node_ui(choice_node& node) {
	ImGui::Text("Choice");
	dirty |= imgui_input_text_multiline<1024>("##Choice", node.text, { 350.0f, ImGui::GetTextLineHeight() * 3.0f });
}

void dialogue_editor_state::update_node_ui(has_item_condition_node& node) {
	ImGui::Text("Has item");
	auto uv = item_definitions().get(node.item.definition_id).uv;
	ImGui::ImageButton((ImTextureID)ui_texture, { 32.0f }, item_uv1(uv, ui_texture), item_uv2(uv, ui_texture), 1);
	bool shown = item_popup_context("##ItemConditionPopup", &node.item);
	if (shown && can_show_context_menu) {
		can_show_context_menu = false;
	}
	ImGui::SameLine();
	ImGui::Text(item_definitions().get(node.item.definition_id).name.c_str());
	ImGui::PushItemWidth(100.0f);
	dirty |= ImGui::Checkbox("Check equipment too##CondCheckEquip", &node.check_equipment_too);
	ImGui::PopItemWidth();
}

void dialogue_editor_state::update_node_ui(stat_condition_node& node) {
	ImGui::Text("Stat condition");
	ImGui::PushItemWidth(100.0f);
	dirty |= select_stat_combo(&node.stat);
	dirty |= ImGui::InputInt("Min", &node.min_value);
	ImGui::SameLine();
	dirty |= ImGui::InputInt("Max", &node.max_value);
	ImGui::PopItemWidth();
}

void dialogue_editor_state::update_node_ui(inventory_effect_node& node) {
	ImGui::Text("Inventory effect");
	int current = (int)(!node.give); // note: "give" is actually 1, but in the combo it's 0
	ImGui::PushItemWidth(100.0f);
	dirty |= ImGui::Combo("##GiveOrTake", &current, "Give\0Take\0\0");
	ImGui::PopItemWidth();
	node.give = (current == 0);
	auto uv = item_definitions().get(node.item.definition_id).uv;
	ImGui::ImageButton((ImTextureID)ui_texture, { 32.0f }, item_uv1(uv, ui_texture), item_uv2(uv, ui_texture), 1);
	bool shown = item_popup_context("##ItemEffectPopup", &node.item);
	if (shown && can_show_context_menu) {
		can_show_context_menu = false;
	}
	ImGui::SameLine();
	ImGui::Text(item_definitions().get(node.item.definition_id).name.c_str());
}

void dialogue_editor_state::update_node_ui(stat_effect_node& node) {
	ImGui::Text("Modify stat");
	ImGui::PushItemWidth(100.0f);
	dirty |= select_stat_combo(&node.stat);
	int assign_or_add = (int)node.assign;
	dirty |= ImGui::Combo(CSTRING("##AssignOrAddStat" << &node), &assign_or_add, "Add\0Assign\0\0");
	ImGui::SameLine();
	node.assign = (assign_or_add != 0);
	dirty |= ImGui::InputInt("Experience", &node.value);
	ImGui::PopItemWidth();
}

void dialogue_editor_state::update_node_ui(warp_effect_node& node) {
	ImGui::Text("Warp effect");
	ImGui::PushItemWidth(100.0f);
	//ImGui::Text("World");
	//ImGui::SameLine();
	//ImGui::Button("Change");
	//world_popup_context(&node.world_id);
	dirty |= ImGui::InputFloat2("Tile", &node.tile.x);
	ImGui::PopItemWidth();
}

void dialogue_editor_state::update_node_ui(var_condition_node& node) {
	ImGui::Text("Compare variable");
	ImGui::PushItemWidth(200.0f);
	dirty |= ImGui::Checkbox("Global", &node.is_global);
	dirty |= imgui_input_text<128>("Name", node.var_name);
	dirty |= ImGui::Combo("Comparison", (int*)&node.comp_operator, "==\0!=\0>\0<\0>=\0<=\0\0");
	dirty |= ImGui::Combo("##OtherType", (int*)&node.other_type, "Value\0Local\0Global\0\0");
	dirty |= imgui_input_text<128>("Value", node.comp_value);
	ImGui::PopItemWidth();
}

void dialogue_editor_state::update_node_ui(modify_var_node& node) {
	ImGui::Text("Modify variable");
	ImGui::PushItemWidth(200.0f);
	dirty |= ImGui::Checkbox("Global", &node.is_global);
	dirty |= imgui_input_text<128>("Name", node.var_name);
	dirty |= ImGui::Combo("Operator", (int*)&node.mod_operator, "Set\0Set Not (Flip)\0Add\0Multiply\0Divide\0\0");
	dirty |= ImGui::Combo("##OtherType", (int*)&node.other_type, "Value\0Local\0Global\0\0");
	dirty |= imgui_input_text<128>("##SetVal", node.mod_value);
	ImGui::PopItemWidth();
}

void dialogue_editor_state::update_node_ui(create_var_node& node) {
	ImGui::Text("Create variable");
	ImGui::PushItemWidth(200.0f);
	dirty |= ImGui::Checkbox("Global", &node.is_global);
	dirty |= ImGui::Checkbox("Persistent", &node.var.is_persistent);
	dirty |= ImGui::Checkbox("Overwrite", &node.overwrite);
	dirty |= ImGui::Combo("Type", (int*)&node.var.type, "String\0Integer\0Float\0Boolean\0\0");
	dirty |= imgui_input_text<128>("Name", node.var.name);
	if (node.var.type == variable_type::string) {
		dirty |= imgui_input_text<128>("Value##Str", node.var.value);
	} else {
		if (node.var.value == "") {
			node.var.value = "0";
		}
		if (node.var.type == variable_type::integer) {
			int as_int = std::stoi(node.var.value);
			dirty |= ImGui::InputInt("Value##Int", &as_int);
			node.var.value = std::to_string(as_int);
		} else if (node.var.type == variable_type::floating) {
			float as_float = std::stof(node.var.value);
			dirty |= ImGui::InputFloat("Value##Float", &as_float);
			node.var.value = std::to_string(as_float);
		} else if (node.var.type == variable_type::boolean) {
			bool as_bool = (std::stoi(node.var.value) != 0);
			dirty |= ImGui::Checkbox("Value##Bool", &as_bool);
			node.var.value = std::to_string(as_bool);
		}
	}
	ImGui::PopItemWidth();
}

void dialogue_editor_state::update_node_ui(var_exists_node& node) {
	ImGui::Text("Does variable exist");
	ImGui::PushItemWidth(200.0f);
	dirty |= ImGui::Checkbox("Global", &node.is_global);
	dirty |= imgui_input_text<128>("Name", node.var_name);
	ImGui::PopItemWidth();
}

void dialogue_editor_state::update_node_ui(delete_var_node& node) {
	ImGui::Text("Delete variable");
	ImGui::PushItemWidth(200.0f);
	dirty |= ImGui::Checkbox("Global", &node.is_global);
	dirty |= imgui_input_text<128>("Name", node.var_name);
	ImGui::PopItemWidth();
}

void dialogue_editor_state::update_node_ui(random_node& node) {
	ImGui::PushItemWidth(100.0f);
	ImGui::Text("Random");
	ImGui::PopItemWidth();
}

void dialogue_editor_state::update_node_ui(random_condition_node& node) {
	ImGui::Text("Random condition");
	dirty |= ImGui::InputInt("% Chance of success", &node.percent);
	node.percent = no::clamp(node.percent, 0, 100);
}

bool dialogue_editor_state::item_popup_context(std::string imgui_id, item_instance* out_item) {
	bool opened = false;
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
	ImGui::PushID(CSTRING("ItemContext" << imgui_id));
	if (ImGui::BeginPopupContextItem(CSTRING(imgui_id << out_item))) {
		for (int type_num = 0; type_num < (int)item_type::total_types; type_num++) {
			ImGui::PushID(CSTRING("Type" << type_num));
			if (ImGui::BeginMenu(CSTRING((item_type)type_num))) {
				auto definitions = item_definitions().of_type((item_type)type_num);
				for (auto& definition : definitions) {
					ImGui::Image((ImTextureID)ui_texture, { 12.0f }, item_uv1(definition.uv, ui_texture), item_uv2(definition.uv, ui_texture));
					ImGui::SameLine();
					if (ImGui::MenuItem(CSTRING(definition.name << "##Item" << definition.id))) {
						out_item->definition_id = definition.id;
						out_item->stack = (int)std::min((int)definition.max_stack, (int)out_item->stack);
						dirty = true;
					}
				}
				ImGui::EndMenu();
			}
			ImGui::PopID();
		}
		if (ImGui::MenuItem(CSTRING("Nothing##NoItem" << imgui_id))) {
			out_item->definition_id = -1;
			out_item->stack = 0;
		}
		if (out_item->definition_id != -1) {
			int stack = (int)out_item->stack;
			ImGui::PushItemWidth(100.0f);
			dirty |= ImGui::InputInt("Stack", &stack);
			ImGui::PopItemWidth();
			out_item->stack = (int64_t)std::max(1, stack);
		} else {
			out_item->stack = 0;
		}
		ImGui::EndPopup();
		opened = true;
	}
	ImGui::PopID();
	ImGui::PopStyleVar();
	return opened;
}

bool dialogue_editor_state::select_stat_combo(int* stat) {
	return ImGui::Combo(CSTRING("Stat##" << stat), stat, "Sword\0Axe\0Spear\0Defense\0Archery\0Stamina\0Fishing\0\0");
}
