#include "ui_main.hpp"
#include "ui_context.hpp"
#include "game.hpp"
#include "trading.hpp"
#include "game_assets.hpp"

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

inventory_view::inventory_view(game_state& game) : game(game) {
	set_ui_uv(background, inventory_uv);
}

inventory_view::~inventory_view() {
	ignore();
}

void inventory_view::on_change(no::vector2i slot) {
	auto player = game.world.objects.character(object_id);
	item_instance item = player->inventory.get(slot);
	int slot_index = slot.y * inventory_container::columns + slot.x;
	if (item.definition_id == -1) {
		slots.erase(slot_index);
	} else {
		slots[slot_index] = {};
		slots[slot_index].item = item;
		set_item_uv(slots[slot_index].rectangle, item.uv_for_stack());
	}
}

void inventory_view::listen(int object_id_) {
	object_id = object_id_;
	auto player = game.world.objects.character(object_id);
	change_event = player->inventory.events.change.listen([this](no::vector2i slot) {
		on_change(slot);
	});
	for (int x = 0; x < inventory_container::columns; x++) {
		for (int y = 0; y < inventory_container::rows; y++) {
			on_change({ x, y });
		}
	}
}

void inventory_view::ignore() {
	if (object_id == -1) {
		return;
	}
	auto player = game.world.objects.character(object_id);
	player->inventory.events.change.ignore(change_event);
	change_event = -1;
	player = nullptr;
}

no::transform2 inventory_view::body_transform() const {
	no::transform2 transform;
	transform.scale = inventory_uv.zw;
	transform.position.x = game.ui_camera.width() - background_uv.z - 2.0f + inventory_offset.x;
	transform.position.y = inventory_offset.y;
	return transform;
}

no::transform2 inventory_view::slot_transform(int index) const {
	no::transform2 transform;
	transform.scale = item_size;
	transform.position.x = game.ui_camera.width() - background_uv.z + 23.0f + (float)(index % 4) * item_grid.x;
	transform.position.y = 132.0f + (float)(index / 4) * item_grid.y;
	return transform;
}

void inventory_view::draw() const {
	no::draw_shape(background, body_transform());
	for (auto& slot : slots) {
		no::draw_shape(slot.second.rectangle, slot_transform(slot.first));
	}
}

void inventory_view::add_context_options() {
	if (!body_transform().collides_with(game.ui_camera.mouse_position(game.mouse()))) {
		return;
	}
	no::vector2i slot = hovered_slot();
	if (slot.x == -1) {
		return;
	}
	auto player = game.world.objects.character(object_id);
	auto item = player->inventory.get(slot);
	if (item.definition_id == -1) {
		return;
	}
	if (is_trading()) {
		add_context_menu_option("Offer " + item.definition().name, [this, item, slot, player] {
			game.send_add_trade_item(item);
			player->inventory.items[slot.y * inventory_container::columns + slot.x] = {};
			player->inventory.events.change.emit(slot);
		});
	} else {
		switch (item.definition().type) {
		case item_type::consumable:
			add_context_menu_option("Eat " + item.definition().name, [this, slot] {
				game.consume_from_inventory(slot);
			});
			break;
		case item_type::equipment:
			add_context_menu_option("Equip " + item.definition().name, [this, slot] {
				game.equip_from_inventory(slot);
			});
			break;
		}
		add_context_menu_option("Drop " + item.definition().name, [this, item, slot, player] {
			item_instance ground_item;
			ground_item.definition_id = item.definition_id;
			player->inventory.remove_to(item.stack, ground_item);
			// todo: drop on ground
		});
	}
}

no::vector2i inventory_view::hovered_slot() const {
	for (auto& slot : slots) {
		if (slot_transform(slot.first).collides_with(game.ui_camera.mouse_position(game.mouse()))) {
			return { slot.first % 4, slot.first / 4 };
		}
	}
	return -1;
}

equipment_view::equipment_view(game_state& game) : game{ game } {
	set_ui_uv(background, equipment_uv);
}

equipment_view::~equipment_view() {
	ignore();
}

void equipment_view::on_change(equipment_slot slot) {
	item_instance item = game.world.objects.character(object_id)->equipment.get(slot);
	if (item.definition_id == -1) {
		slots.erase(slot);
	} else {
		slots[slot] = {};
		slots[slot].item = item;
		set_item_uv(slots[slot].rectangle, item.uv_for_stack());
	}
}

void equipment_view::listen(int object_id_) {
	object_id = object_id_;
	auto player = game.world.objects.character(object_id);
	change_event = player->equipment.events.change.listen([this](equipment_slot slot) {
		on_change(slot);
	});
	for (int i = 0; i < (int)equipment_slot::total_slots; i++) {
		on_change((equipment_slot)i);
	}
}

void equipment_view::ignore() {
	if (object_id == -1) {
		return;
	}
	auto player = game.world.objects.character(object_id);
	player->equipment.events.change.ignore(change_event);
	change_event = -1;
	player = nullptr;
}

no::transform2 equipment_view::body_transform() const {
	no::transform2 transform;
	transform.scale = inventory_uv.zw;
	transform.position.x = game.ui_camera.width() - background_uv.z - 2.0f + inventory_offset.x;
	transform.position.y = inventory_offset.y;
	return transform;
}

no::transform2 equipment_view::slot_transform(equipment_slot slot) const {
	no::transform2 transform;
	transform.scale = item_size;
	transform.position.x = game.ui_camera.width() - background_uv.z + 26.0f;
	transform.position.y = 141.0f;
	switch (slot) {
	case equipment_slot::left_hand:
		transform.position.x += 2.0f * (item_grid.x + 14.0f);
		transform.position.y += 1.0f * (item_grid.y + 14.0f);
		break;
	case equipment_slot::right_hand:
		transform.position.y += 1.0f * (item_grid.y + 14.0f);
		break;
	case equipment_slot::legs:
		transform.position.x += 1.0f * (item_grid.x + 14.0f);
		transform.position.y += 2.0f * (item_grid.y + 14.0f);
		break;
	default:
		break;
	}
	return transform;
}

void equipment_view::draw() const {
	no::draw_shape(background, body_transform());
	for (auto& slot : slots) {
		no::draw_shape(slot.second.rectangle, slot_transform(slot.first));
	}
}

void equipment_view::add_context_options() {
	if (!body_transform().collides_with(game.ui_camera.mouse_position(game.mouse()))) {
		return;
	}
	equipment_slot slot = hovered_slot();
	if (slot == equipment_slot::none) {
		return;
	}
	auto player = game.world.objects.character(object_id);
	auto item = slots[slot].item;
	if (item.definition_id == -1) {
		return;
	}
	auto definition = item_definitions().get(item.definition_id);
	add_context_menu_option("Unequip", [this, slot] {
		game.unequip_to_inventory(slot);
	});
}

equipment_slot equipment_view::hovered_slot() const {
	for (auto& slot : slots) {
		if (slot_transform(slot.first).collides_with(game.ui_camera.mouse_position(game.mouse()))) {
			return slot.first;
		}
	}
	return equipment_slot::none;
}

user_interface_view::user_interface_view(game_state& game) : 
	game(game), inventory{ game }, equipment{ game }, stats{ game }, minimap{ game.world } {
	set_ui_uv(background, background_uv);
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

bool user_interface_view::is_mouse_over_inventory() const {
	return inventory.body_transform().collides_with(game.ui_camera.mouse_position(game.mouse()));
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
	equipment_event = player->events.equip.listen([this](const item_instance& event) {
		
	});
	press_event_id = game.mouse().press.listen([this](const no::mouse::press_message& event) {
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
					tabs.active = i;
					break;
				}
			}
		} else if (event.button == no::mouse::button::right) {
			create_context();
		}
	});
	cursor_icon_id = game.mouse().icon.listen([this] {
		switch (tabs.active) {
		case 0:
			if (inventory.hovered_slot().x != -1) {
				game.mouse().set_icon(no::mouse::cursor::pointer);
				return;
			}
			break;
		case 1:
			if (equipment.hovered_slot() != equipment_slot::none) {
				game.mouse().set_icon(no::mouse::cursor::pointer);
				return;
			}
			break;
		}
		for (int i = 0; i < 4; i++) {
			if (is_tab_hovered(i)) {
				game.mouse().set_icon(no::mouse::cursor::pointer);
				return;
			}
		}
		game.mouse().set_icon(no::mouse::cursor::arrow);
	});
	inventory.listen(object_id);
	equipment.listen(object_id);
}

void user_interface_view::ignore() {
	if (object_id == -1) {
		return;
	}
	auto player = game.world.objects.character(object_id);
	inventory.ignore();
	equipment.ignore();
	player->events.equip.ignore(equipment_event);
	equipment_event = -1;
	game.mouse().press.ignore(press_event_id);
	game.mouse().icon.ignore(cursor_icon_id);
	press_event_id = -1;
	player = nullptr;
}

void user_interface_view::update() {
	minimap.transform.position = { 109.0f, 12.0f };
	minimap.transform.position.x += game.ui_camera.width() - background_uv.z - 2.0f;
	minimap.transform.scale = 64.0f;
	minimap.transform.rotation = game.world_camera().transform.rotation.y;
	minimap.update(game.world.my_player().object.tile());
	update_trading_ui();
}

void user_interface_view::draw() {
	no::bind_shader(shaders().sprite.id);
	no::set_shader_view_projection(game.ui_camera);
	shaders().sprite.color.set(no::vector4f{ 1.0f });
	minimap.draw();
	no::bind_texture(sprites().ui);
	no::transform2 transform;
	transform.scale = background_uv.zw;
	transform.position.x = game.ui_camera.width() - transform.scale.x - 2.0f;
	no::draw_shape(background, transform);
	switch (tabs.active) {
	case 0:
		inventory.draw();
		break;
	case 1:
		equipment.draw();
		break;
	case 2:
		break;
	case 3:
		stats.draw();
		break;
	}
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
	switch (tabs.active) {
	case 0: inventory.add_context_options(); break;
	case 1: equipment.add_context_options(); break;
	}
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
					game.chat.add("", definition.description);
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
