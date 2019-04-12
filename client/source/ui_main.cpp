#include "ui_main.hpp"
#include "ui_context.hpp"
#include "game.hpp"
#include "trading.hpp"
#include "game_assets.hpp"
#include "ui_tab_inventory.hpp"
#include "ui_tab_equipment.hpp"
#include "ui_tab_stats.hpp"

#include "assets.hpp"
#include "surface.hpp"

void set_ui_uv(no::rectangle& rectangle, no::vector2f uv, no::vector2f uv_size) {
	rectangle.set_tex_coords(uv.x / ui_size.x, uv.y / ui_size.y, uv_size.x / ui_size.x, uv_size.y / ui_size.y);
}

void set_ui_uv(no::rectangle& rectangle, no::vector4f uv) {
	set_ui_uv(rectangle, uv.xy, uv.zw);
}

void set_item_uv(no::rectangle& rectangle, no::vector2f uv) {
	set_ui_uv(rectangle, uv, item_size);
}

user_interface_view::user_interface_view(game_state& game) : game{ game } {
	set_ui_uv(tabs.background, tab_background_uv, tab_size);
	set_ui_uv(tabs.inventory, tab_inventory_uv, tab_size);
	set_ui_uv(tabs.equipment, tab_equipment_uv, tab_size);
	set_ui_uv(tabs.quests, tab_quests_uv, tab_size);
	set_ui_uv(tabs.stats, tab_stats_uv, tab_size);
}

user_interface_view::~user_interface_view() {
	ignore();
	close_context_menu();
}

bool user_interface_view::is_mouse_over() const {
	no::transform2 transform;
	transform.scale = background_uv.zw;
	transform.position.x = game.ui_camera.width() - transform.scale.x - 2.0f;
	return transform.collides_with(game.ui_camera.mouse_position(game.mouse()));
}

bool user_interface_view::is_tab_hovered(int index) const {
	return tab_transform(index).collides_with(game.ui_camera.mouse_position(game.mouse()));
}

bool user_interface_view::is_mouse_over_any() const {
	return is_mouse_over() || is_mouse_over_context_menu();
}

void user_interface_view::listen(int object_id_) {
	object_id = object_id_;
	auto player = game.world.objects.character(object_id);
	press_event_id = game.mouse().press.listen([this](const no::mouse::press_message & event) {
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
					switch (tabs.active) {
					case 0: hide_inventory_tab(); break;
					case 1: hide_equipment_tab(); break;
					case 3: hide_stats_tab(); break;
					}
					tabs.active = i;
					switch (tabs.active) {
					case 0: show_inventory_tab(game); break;
					case 1: show_equipment_tab(game); break;
					case 3: show_stats_tab(game); break;
					}
					break;
				}
			}
		} else if (event.button == no::mouse::button::right) {
			create_context();
		}
	});
	cursor_icon_id = game.mouse().icon.listen([this] {
		for (int i = 0; i < 4; i++) {
			if (is_tab_hovered(i)) {
				game.mouse().set_icon(no::mouse::cursor::pointer);
				return;
			}
		}
		game.mouse().set_icon(no::mouse::cursor::arrow);
	});
	switch (tabs.active) {
	case 0: show_inventory_tab(game); break;
	case 1: show_equipment_tab(game); break;
	case 3: show_stats_tab(game); break;
	}
}

void user_interface_view::ignore() {
	if (object_id == -1) {
		return;
	}
	auto player = game.world.objects.character(object_id);
	game.mouse().press.ignore(press_event_id);
	game.mouse().icon.ignore(cursor_icon_id);
	press_event_id = -1;
	cursor_icon_id = -1;
	switch (tabs.active) {
	case 0: hide_inventory_tab(); break;
	case 1: hide_equipment_tab(); break;
	case 3: hide_stats_tab(); break;
	}
}

void user_interface_view::update() {

}

void user_interface_view::draw() {
	draw_inventory_tab();
	draw_equipment_tab();
	draw_stats_tab();

	no::bind_texture(sprites().ui);
	draw_tabs();
	draw_trading_ui();
	shaders().sprite.color.set(no::vector4f{ 1.0f });
	no::bind_texture(sprites().ui);
	draw_context_menu();
}

void user_interface_view::draw_tabs() const {
	draw_tab(0, tabs.inventory);
	draw_tab(1, tabs.equipment);
	draw_tab(2, tabs.quests);
	draw_tab(3, tabs.stats);
}

void user_interface_view::draw_tab(int index, const no::rectangle& tab) const {
	no::set_shader_model(tab_transform(index));
	bool hover = is_tab_hovered(index);
	if (hover) {
		shaders().sprite.color.set({ 1.0f, 0.7f, 0.3f, 1.0f });
	}
	if (tabs.active == index) {
		shaders().sprite.color.set({ 1.0f, 0.2f, 0.2f, 1.0f });
	}
	tabs.background.bind();
	tabs.background.draw();
	shaders().sprite.color.set(no::vector4f{ 1.0f });
	tab.bind();
	tab.draw();
}

no::transform2 user_interface_view::tab_transform(int index) const {
	no::transform2 transform;
	transform.position = { 429.0f, 146.0f };
	transform.position -= background_uv.xy;
	transform.position.x += game.ui_camera.width() - background_uv.z - 2.0f + (tab_size.x + 4.0f) * (float)index;
	transform.scale = tab_size;
	return transform;
}

void user_interface_view::create_context() {
	open_context_menu(game);
	add_inventory_context_menu_options();
	add_equipment_context_menu_options();
	add_trading_context_options();
	if (!is_mouse_over_inventory()) {
		auto player = game.world.my_player();
		no::vector2i tile = game.hovered_tile();
		if (game.world.can_fish_at(player.object.tile(), tile)) {
			add_context_menu_option("Cast fishing rod", [this, tile] {
				game.send_start_fishing(tile);
			});
		}
		auto& objects = game.world.objects;
		objects.for_each([this, tile](game_object* object) {
			if (object->tile() != tile) {
				return;
			}
			auto& definition = object->definition();
			std::string name = definition.name;
			if (definition.type == game_object_type::character) {
				auto character = game.world.objects.character(object->instance_id);
				if (!character->name.empty()) {
					name = character->name;
				}
				int target_id = object->instance_id; // objects array can be resized
				if (character->stat(stat_type::health).real() > 0) {
					add_context_menu_option("Attack " + name, [this, target_id] {
						game.start_combat(target_id);
					});
				}
				if (definition.id == 1) {
					add_context_menu_option("Trade with " + name, [this, target_id] {
						game.send_trade_request(target_id);
					});
				}
			}
			if (definition.script_id.dialogue != -1) {
				std::string option_name = "Use ";
				if (definition.type == game_object_type::character) {
					option_name = "Talk to ";
				}
				add_context_menu_option(option_name + name, [&] {
					game.start_dialogue(definition.script_id.dialogue);
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
