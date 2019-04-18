#include "ui_tabs.hpp"
#include "ui_context.hpp"
#include "ui_trading.hpp"
#include "ui_tab_inventory.hpp"
#include "ui_tab_equipment.hpp"
#include "ui_tab_stats.hpp"
#include "chat.hpp"
#include "game.hpp"
#include "game_assets.hpp"

struct tabs_view {

	game_state& game;
	no::rectangle background;
	no::rectangle inventory;
	no::rectangle equipment;
	no::rectangle quests;
	no::rectangle stats;
	int active = 0;
	int object_id = -1;
	int press_event_id = -1;
	int cursor_icon_id = -1;

	tabs_view(game_state& game) : game{ game } {}

};

static tabs_view* tabs = nullptr;

void set_ui_uv(no::rectangle& rectangle, no::vector2f uv, no::vector2f uv_size) {
	rectangle.set_tex_coords(uv.x / ui_size.x, uv.y / ui_size.y, uv_size.x / ui_size.x, uv_size.y / ui_size.y);
}

void set_ui_uv(no::rectangle& rectangle, no::vector4f uv) {
	set_ui_uv(rectangle, uv.xy, uv.zw);
}

void set_item_uv(no::rectangle& rectangle, no::vector2f uv) {
	set_ui_uv(rectangle, uv, item_size);
}

no::transform2 tab_body_transform() {
	no::transform2 transform;
	transform.scale = { 138.0f, 205.0f };
	transform.position.x = tabs->game.ui_camera.width() - background_uv.z - 2.0f + inventory_offset.x;
	transform.position.y = inventory_offset.y;
	return transform;
}

static no::transform2 tab_transform(int index) {
	no::transform2 transform;
	transform.position = { 429.0f, 146.0f };
	transform.position -= background_uv.xy;
	transform.position.x += tabs->game.ui_camera.width() - background_uv.z - 2.0f + (tab_size.x + 4.0f) * (float)index;
	transform.scale = tab_size;
	return transform;
}

static bool is_tab_hovered(int index) {
	return tab_transform(index).collides_with(tabs->game.ui_camera.mouse_position(tabs->game.mouse()));
}

void draw_tab(int index, const no::rectangle& tab) {
	no::set_shader_model(tab_transform(index));
	bool hover = is_tab_hovered(index);
	if (hover) {
		shaders().sprite.color.set({ 1.0f, 0.7f, 0.3f, 1.0f });
	}
	if (tabs->active == index) {
		shaders().sprite.color.set({ 1.0f, 0.2f, 0.2f, 1.0f });
	}
	tabs->background.bind();
	tabs->background.draw();
	shaders().sprite.color.set(no::vector4f{ 1.0f });
	tab.bind();
	tab.draw();
}

void show_tabs(game_state& game) {
	hide_tabs();
	tabs = new tabs_view{ game };
	set_ui_uv(tabs->background, tab_background_uv, tab_size);
	set_ui_uv(tabs->inventory, tab_inventory_uv, tab_size);
	set_ui_uv(tabs->equipment, tab_equipment_uv, tab_size);
	set_ui_uv(tabs->quests, tab_quests_uv, tab_size);
	set_ui_uv(tabs->stats, tab_stats_uv, tab_size);
}

void hide_tabs() {
	disable_tabs();
	close_context_menu();
	delete tabs;
	tabs = nullptr;
}

void update_tabs() {

}

void draw_tabs() {
	if (!tabs) {
		return;
	}
	draw_inventory_tab();
	draw_equipment_tab();
	draw_stats_tab();
	no::bind_texture(sprites().ui);
	draw_tab(0, tabs->inventory);
	draw_tab(1, tabs->equipment);
	draw_tab(2, tabs->quests);
	draw_tab(3, tabs->stats);
	draw_trading_ui();
	shaders().sprite.color.set(no::vector4f{ 1.0f });
	no::bind_texture(sprites().ui);
	draw_context_menu();
}

void add_tabs_context_menu_options() {
	open_context_menu(tabs->game);
	add_inventory_context_menu_options();
	add_equipment_context_menu_options();
	add_trading_context_options();
	if (!is_mouse_over_tab_body()) {
		auto player = tabs->game.world.my_player();
		no::vector2i tile = tabs->game.hovered_tile();
		if (tabs->game.world.can_fish_at(player.object.tile(), tile)) {
			add_context_menu_option("Cast fishing rod", [tile] {
				tabs->game.send_start_fishing(tile);
			});
		}
		auto& objects = tabs->game.world.objects;
		objects.for_each([tile](game_object * object) {
			if (object->tile() != tile) {
				return;
			}
			auto& definition = object->definition();
			std::string name = definition.name;
			if (definition.type == game_object_type::character) {
				auto character = tabs->game.world.objects.character(object->instance_id);
				if (!character->name.empty()) {
					name = character->name;
				}
				if (character->stat(stat_type::health).real() > 0) {
					name += " (Lv. " + std::to_string(character->combat_level()) + ")";
				}
				int target_id = object->instance_id; // objects array can be resized
				if (character->stat(stat_type::health).real() > 0) {
					add_context_menu_option("Attack " + name, [target_id] {
						tabs->game.start_combat(target_id);
					});
				}
				if (definition.id == 1) {
					add_context_menu_option("Trade with " + name, [target_id] {
						tabs->game.send_trade_request(target_id);
					});
				}
			}
			if (definition.script_id.dialogue != -1) {
				std::string option_name = "Use ";
				if (definition.type == game_object_type::character) {
					option_name = "Talk to ";
				}
				add_context_menu_option(option_name + name, [&] {
					tabs->game.start_dialogue(definition.script_id.dialogue);
				});
			}
			if (!definition.description.empty()) {
				add_context_menu_option("Examine " + name, [&] {
					add_chat_message("", definition.description);
				});
			}
		});
	}
	if (context_menu_option_count() == 0) {
		close_context_menu();
	} else {
		add_context_menu_option("Cancel", [] {});
	}
}

void enable_tabs() {
	if (!tabs) {
		return;
	}
	tabs->object_id = tabs->game.world.my_player().object.instance_id;
	auto player = tabs->game.world.objects.character(tabs->object_id);
	tabs->press_event_id = tabs->game.mouse().press.listen([](const no::mouse::press_message & event) {
		if (event.button == no::mouse::button::left) {
			for (int i = 0; i < context_menu_option_count(); i++) {
				if (is_mouse_over_context_menu_option(i)) {
					trigger_context_menu_option(i);
					close_context_menu();
					return; // don't let through tab click
				}
			}
			close_context_menu();
			for (int i = 0; i < 4; i++) {
				if (is_tab_hovered(i)) {
					switch (tabs->active) {
					case 0: hide_inventory_tab(); break;
					case 1: hide_equipment_tab(); break;
					case 3: hide_stats_tab(); break;
					}
					tabs->active = i;
					switch (tabs->active) {
					case 0: show_inventory_tab(tabs->game); break;
					case 1: show_equipment_tab(tabs->game); break;
					case 3: show_stats_tab(tabs->game); break;
					}
					break;
				}
			}
		} else if (event.button == no::mouse::button::right) {
			add_tabs_context_menu_options();
		}
	});
	tabs->cursor_icon_id = tabs->game.mouse().icon.listen([] {
		for (int i = 0; i < 4; i++) {
			if (is_tab_hovered(i)) {
				tabs->game.mouse().set_icon(no::mouse::cursor::pointer);
				return;
			}
		}
		tabs->game.mouse().set_icon(no::mouse::cursor::arrow);
	});
	switch (tabs->active) {
	case 0: show_inventory_tab(tabs->game); break;
	case 1: show_equipment_tab(tabs->game); break;
	case 3: show_stats_tab(tabs->game); break;
	}
}

void disable_tabs() {
	if (!tabs) {
		return;
	}
	auto player = tabs->game.world.objects.character(tabs->object_id);
	tabs->game.mouse().press.ignore(tabs->press_event_id);
	tabs->game.mouse().icon.ignore(tabs->cursor_icon_id);
	tabs->press_event_id = -1;
	tabs->cursor_icon_id = -1;
	switch (tabs->active) {
	case 0: hide_inventory_tab(); break;
	case 1: hide_equipment_tab(); break;
	case 3: hide_stats_tab(); break;
	}
}

bool is_mouse_over_tabs() {
	if (!tabs) {
		return false;
	}
	no::transform2 transform;
	transform.scale = background_uv.zw;
	transform.position.x = tabs->game.ui_camera.width() - transform.scale.x - 2.0f;
	return transform.collides_with(tabs->game.ui_camera.mouse_position(tabs->game.mouse()));
}

bool is_mouse_over_tab_body() {
	if (!tabs) {
		return false;
	}
	return tab_body_transform().collides_with(tabs->game.ui_camera.mouse_position(tabs->game.mouse()));
}
