#include "ui_tab_inventory.hpp"
#include "ui_tabs.hpp"
#include "ui_context.hpp"
#include "ui_trading.hpp"
#include "game.hpp"
#include "item.hpp"

const no::vector4f inventory_uv = { 200.0f, 128.0f, 138.0f, 205.0f };

struct item_slot {
	no::rectangle rectangle;
	item_instance item;
};

struct inventory_view {

	game_state& game;
	int object_id = -1;
	no::rectangle background;
	std::unordered_map<int, item_slot> slots;
	int change_event = -1;
	int cursor_icon_id = -1;

	inventory_view(game_state& game) : game{ game } {}

};

static inventory_view* inventory = nullptr;

static void on_change(no::vector2i slot) {
	auto player = inventory->game.world.objects.character(inventory->object_id);
	item_instance item = player->inventory.get(slot);
	int slot_index = slot.y * inventory_container::columns + slot.x;
	if (item.definition_id == -1) {
		inventory->slots.erase(slot_index);
	} else {
		inventory->slots[slot_index] = {};
		inventory->slots[slot_index].item = item;
		set_item_uv(inventory->slots[slot_index].rectangle, item.uv_for_stack());
	}
}

static no::transform2 inventory_transform() {
	no::transform2 transform = tab_body_transform();
	transform.position.x += 3.0f;
	transform.position.y += 5.0f;
	return transform;
}

static no::transform2 slot_transform(int index) {
	no::transform2 transform;
	transform.scale = item_size;
	transform.position = inventory_transform().position + 1.0f;
	transform.position.x += (float)(index % 4) * item_grid.x;
	transform.position.y += (float)(index / 4) * item_grid.y;
	return transform;
}

static no::vector2i hovered_slot() {
	for (auto& slot : inventory->slots) {
		if (slot_transform(slot.first).collides_with(inventory->game.ui_camera_2x.mouse_position(inventory->game.mouse()))) {
			return { slot.first % 4, slot.first / 4 };
		}
	}
	return -1;
}

void show_inventory_tab(game_state& game) {
	hide_inventory_tab();
	inventory = new inventory_view{ game };
	set_ui_uv(inventory->background, inventory_uv);
	inventory->object_id = game.world.my_player().object.instance_id;
	auto player = game.world.objects.character(inventory->object_id);
	inventory->change_event = player->inventory.events.change.listen([](no::vector2i slot) {
		on_change(slot);
	});
	for (int x = 0; x < inventory_container::columns; x++) {
		for (int y = 0; y < inventory_container::rows; y++) {
			on_change({ x, y });
		}
	}
	inventory->cursor_icon_id = game.mouse().icon.listen([] {
		if (hovered_slot().x != -1) {
			inventory->game.mouse().set_icon(no::mouse::cursor::pointer);
		}
	});
}

void hide_inventory_tab() {
	if (!inventory) {
		return;
	}
	auto player = inventory->game.world.objects.character(inventory->object_id);
	player->inventory.events.change.ignore(inventory->change_event);
	inventory->change_event = -1;
	inventory->game.mouse().icon.ignore(inventory->cursor_icon_id);
	delete inventory;
	inventory = nullptr;
}

void update_inventory_tab() {

}

void draw_inventory_tab() {
	if (!inventory) {
		return;
	}
	no::draw_shape(inventory->background, inventory_transform());
	for (auto& slot : inventory->slots) {
		no::draw_shape(slot.second.rectangle, slot_transform(slot.first));
	}
}

void add_inventory_context_menu_options() {
	if (!inventory || !is_mouse_over_tab_body()) {
		return;
	}
	no::vector2i slot = hovered_slot();
	if (slot.x == -1) {
		return;
	}
	auto player = inventory->game.world.objects.character(inventory->object_id);
	auto item = player->inventory.get(slot);
	if (item.definition_id == -1) {
		return;
	}
	if (is_trading()) {
		add_context_menu_option("Offer " + item.definition().name, [item, slot, player] {
			inventory->game.send_add_trade_item(item);
			player->inventory.items[slot.y * inventory_container::columns + slot.x] = {};
			player->inventory.events.change.emit(slot);
		});
	} else {
		switch (item.definition().type) {
		case item_type::consumable:
			add_context_menu_option("Eat " + item.definition().name, [slot] {
				inventory->game.consume_from_inventory(slot);
			});
			break;
		case item_type::equipment:
			add_context_menu_option("Equip " + item.definition().name, [slot] {
				inventory->game.equip_from_inventory(slot);
			});
			break;
		}
		add_context_menu_option("Drop " + item.definition().name, [item, slot, player] {
			item_instance ground_item;
			ground_item.definition_id = item.definition_id;
			player->inventory.remove_to(item.stack, ground_item);
			// todo: drop on ground
		});
	}
}
