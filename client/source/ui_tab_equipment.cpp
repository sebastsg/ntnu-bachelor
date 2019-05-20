#include "ui_tab_equipment.hpp"
#include "ui_tabs.hpp"
#include "ui_context.hpp"
#include "game.hpp"

const no::vector4f equipment_uv = { 199.0f, 336.0f, 138.0f, 205.0f };

struct item_slot {
	no::rectangle rectangle;
	item_instance item;
};

struct equipment_view {

	game_state& game;
	int object_id = -1;
	no::rectangle background;
	std::unordered_map<equipment_slot, item_slot> slots;
	int change_event = -1;
	int cursor_icon_id = -1;

	equipment_view(game_state& game) : game{ game } {}

};

static equipment_view* equipment = nullptr;

static void on_change(equipment_slot slot) {
	item_instance item = equipment->game.world.objects.character(equipment->object_id)->equipment.get(slot);
	if (item.definition_id == -1) {
		equipment->slots.erase(slot);
	} else {
		equipment->slots[slot] = {};
		equipment->slots[slot].item = item;
		set_item_uv(equipment->slots[slot].rectangle, item.uv_for_stack());
	}
}

static no::transform2 equipment_transform() {
	no::transform2 transform = tab_body_transform();
	transform.position.x += 3.0f;
	transform.position.y += 6.0f;
	return transform;
}

static no::transform2 slot_transform(equipment_slot slot) {
	no::transform2 transform;
	transform.scale = item_size;
	transform.position = equipment_transform().position;
	transform.position.x += 5.0f;
	transform.position.y += 12.0f;
	switch (slot) {
	case equipment_slot::left_hand:
		transform.position.x += 2.0f * (item_grid.x + 14.0f);
		transform.position.y += 1.0f * (item_grid.y + 14.0f);
		break;
	case equipment_slot::right_hand:
		transform.position.y += 1.0f * (item_grid.y + 14.0f);
		break;
	case equipment_slot::head:
		transform.position.x += 1.0f * (item_grid.x + 14.0f);
		break;
	case equipment_slot::body:
		transform.position.x += 1.0f * (item_grid.x + 14.0f);
		transform.position.y += 1.0f * (item_grid.y + 14.0f);
		break;
	case equipment_slot::legs:
		transform.position.x += 1.0f * (item_grid.x + 14.0f);
		transform.position.y += 2.0f * (item_grid.y + 14.0f);
		break;
	case equipment_slot::feet:
		transform.position.x += 1.0f * (item_grid.x + 14.0f);
		transform.position.y += 3.0f * (item_grid.y + 14.0f);
		break;
	default:
		return {};
	}
	return transform;
}

static equipment_slot hovered_slot() {
	for (auto& slot : equipment->slots) {
		if (slot_transform(slot.first).collides_with(equipment->game.ui_camera_2x.mouse_position(equipment->game.mouse()))) {
			return slot.first;
		}
	}
	return equipment_slot::none;
}

void show_equipment_tab(game_state& game) {
	hide_equipment_tab();
	equipment = new equipment_view{ game };
	set_ui_uv(equipment->background, equipment_uv);
	equipment->object_id = game.world.my_player().object.instance_id;
	auto player = game.world.objects.character(equipment->object_id);
	equipment->change_event = player->equipment.events.change.listen([](equipment_slot slot) {
		on_change(slot);
	});
	for (int i = 0; i < (int)equipment_slot::total_slots; i++) {
		on_change((equipment_slot)i);
	}
	equipment->cursor_icon_id = game.mouse().icon.listen([] {
		if (hovered_slot() != equipment_slot::none) {
			equipment->game.mouse().set_icon(no::mouse::cursor::pointer);
		}
	});
}

void hide_equipment_tab() {
	if (!equipment) {
		return;
	}
	auto player = equipment->game.world.objects.character(equipment->object_id);
	player->equipment.events.change.ignore(equipment->change_event);
	equipment->change_event = -1;
	equipment->game.mouse().icon.ignore(equipment->cursor_icon_id);
	delete equipment;
	equipment = nullptr;
}

void update_equipment_tab() {

}

void draw_equipment_tab() {
	if (!equipment) {
		return;
	}
	no::draw_shape(equipment->background, equipment_transform());
	for (auto& slot : equipment->slots) {
		no::draw_shape(slot.second.rectangle, slot_transform(slot.first));
	}
}

void add_equipment_context_menu_options() {
	if (!equipment || !is_mouse_over_tab_body()) {
		return;
	}
	equipment_slot slot = hovered_slot();
	if (slot == equipment_slot::none) {
		return;
	}
	auto player = equipment->game.world.objects.character(equipment->object_id);
	auto item = equipment->slots[slot].item;
	if (item.definition_id == -1) {
		return;
	}
	auto definition = item_definitions().get(item.definition_id);
	add_context_menu_option("Unequip " + definition.name, [slot] {
		equipment->game.unequip_to_inventory(slot);
	});
}
