#include "ui_trading.hpp"
#include "ui_context.hpp"
#include "game.hpp"
#include "game_assets.hpp"
#include "ui.hpp"
#include "ui_tabs.hpp"
#include "chat.hpp"

const no::vector4f trade_background_uv{ 34.0f, 612.0f, 358.0f, 248.0f };
const no::vector4f button_uv{ 442.0f, 34.0f, 92.0f, 25.0f };
const no::vector4f grid_uv = { 200.0f, 128.0f, 138.0f, 205.0f };

struct item_slot {
	no::rectangle rectangle;
	item_instance item;
};

struct trading_view {

	game_state& game;
	int trader_id = -1;
	int cursor_icon_id = -1;

	no::rectangle background;
	no::rectangle grid;
	no::button accept;
	no::button cancel;
	no::transform2 transform;

	no::text_view waiting_label;
	bool waiting = false;
	no::text_view other_accepted_label;
	bool other_accepted = false;

	inventory_container left;
	inventory_container right;
	std::vector<item_slot> left_slots;
	std::vector<item_slot> right_slots;

	trading_view(game_state& game, int trader_id);
	~trading_view();

	void update();
	void draw();

	no::transform2 slot_transform(no::vector2f position, int index) const;
	no::vector2i hovered_slot(std::vector<item_slot>& slots, no::vector2f position) const;
	void add_item(inventory_container& items, std::vector<item_slot>& slots, item_instance item);
	void remove_item(inventory_container& items, std::vector<item_slot>& slots, no::vector2i slot);
	void add_context_options(no::vector2f position, std::vector<item_slot>& slots, bool mine);
	void finish();

};

trading_view::trading_view(game_state& game_, int trader_id) :
	game{ game_ }, 
	trader_id{ trader_id }, 
	accept{ game_, game_.ui_camera_2x },
	cancel{ game_, game_.ui_camera_2x },
	other_accepted_label{ game_, game_.ui_camera_2x },
	waiting_label{ game_, game_.ui_camera_2x } {
	set_ui_uv(background, trade_background_uv);
	set_ui_uv(grid, grid_uv);
	transform.scale = trade_background_uv.zw;
	accept.transition.enabled = false;
	cancel.transition.enabled = false;
	accept.animation.frames = 1;
	cancel.animation.frames = 1;
	accept.set_tex_coords(button_uv.xy / 1024.0f, button_uv.zw / 1024.0f);
	cancel.set_tex_coords(button_uv.xy / 1024.0f, button_uv.zw / 1024.0f);
	accept.transform.scale = button_uv.zw;
	cancel.transform.scale = button_uv.zw;
	accept.label.render(fonts().leo_10, "Accept");
	cancel.label.render(fonts().leo_10, "Cancel");
	accept.events.click.listen([this] {
		game.send_finish_trading(true);
		waiting = true;
		finish();
	});
	cancel.events.click.listen([this] {
		game.send_finish_trading(false);
	});
	other_accepted_label.render(fonts().leo_10, "Other player has accepted this trade");
	waiting_label.render(fonts().leo_10, "Waiting for other player...");

	cursor_icon_id = game.mouse().icon.listen([this] {
		if (accept.is_hovered() || cancel.is_hovered()) {
			game.mouse().set_icon(no::mouse::cursor::pointer);
		}
	});
}

trading_view::~trading_view() {
	game.mouse().icon.ignore(cursor_icon_id);
}

void trading_view::update() {
	transform.position = game.ui_camera_2x.size() / 2.0f - transform.scale / 2.0f;
	accept.transform.position.x = transform.position.x + transform.scale.x / 2.0f - 4.0f - 122.0f;
	cancel.transform.position.x = transform.position.x + transform.scale.x / 2.0f + 4.0f + 34.0f;
	accept.transform.position.y = transform.position.y + transform.scale.y - accept.transform.scale.y - 8.0f;
	cancel.transform.position.y = transform.position.y + transform.scale.y - cancel.transform.scale.y - 8.0f;
	if (!waiting) {
		accept.update();
	}
	cancel.update();
	other_accepted_label.transform.position.x = transform.position.x + transform.scale.x / 2.0f - other_accepted_label.transform.scale.x / 2.0f;
	other_accepted_label.transform.position.y = transform.position.y + transform.scale.y + 8.0f;
	waiting_label.transform.position.x = transform.position.x + transform.scale.x / 2.0f - waiting_label.transform.scale.x / 2.0f;
	waiting_label.transform.position.y = transform.position.y + transform.scale.y + 8.0f;
}

void trading_view::draw() {
	accept.color = shaders().sprite.color;
	cancel.color = shaders().sprite.color;
	no::draw_shape(background, transform);
	no::transform2 grid_transform;
	grid_transform.position = transform.position + no::vector2f{ 32.0f, 10.0f };
	grid_transform.scale = grid_uv.zw;
	no::draw_shape(grid, grid_transform);
	grid_transform.position.x += transform.scale.x / 2.0f - 24.0f;
	no::draw_shape(grid, grid_transform);
	grid_transform.position.x -= transform.scale.x / 2.0f - 24.0f;
	int index = 0;
	no::vector2f position = grid_transform.position + 1.0f;
	for (auto& slot : left_slots) {
		if (slot.item.definition_id != -1) {
			no::draw_shape(slot.rectangle, slot_transform(position, index));
		}
		index++;
	}
	index = 0;
	grid_transform.position.x += transform.scale.x / 2.0f - 24.0f;
	position.x = grid_transform.position.x + grid_transform.scale.x - item_grid.x * 4.0f - 1.0f;
	for (auto& slot : right_slots) {
		if (slot.item.definition_id != -1) {
			no::draw_shape(slot.rectangle, slot_transform(position, index));
		}
		index++;
	}
	if (!waiting) {
		if (accept.is_hovered()) {
			if (accept.is_pressed()) {
				shaders().sprite.color.set({ 1.0f, 0.2f, 0.2f, 1.0f });
			} else {
				shaders().sprite.color.set({ 1.0f, 0.7f, 0.3f, 1.0f });
			}
		}
		accept.draw_button();
		shaders().sprite.color.set(no::vector4f{ 1.0f });
	}
	if (cancel.is_hovered()) {
		if (cancel.is_pressed()) {
			shaders().sprite.color.set({ 1.0f, 0.2f, 0.2f, 1.0f });
		} else {
			shaders().sprite.color.set({ 1.0f, 0.7f, 0.3f, 1.0f });
		}
	}
	cancel.draw_button();
	if (!waiting) {
		accept.draw_label();
	}
	cancel.draw_label();
	shaders().sprite.color.set(no::vector4f{ 1.0f });
	if (other_accepted) {
		other_accepted_label.draw(shapes().rectangle);
	} else if (waiting) {
		waiting_label.draw(shapes().rectangle);
	}
}

no::transform2 trading_view::slot_transform(no::vector2f position, int index) const {
	no::transform2 transform;
	transform.scale = item_size;
	transform.position.x = position.x + (float)(index % 4) * item_grid.x;
	transform.position.y = position.y + (float)(index / 4) * item_grid.y;
	return transform;
}

no::vector2i trading_view::hovered_slot(std::vector<item_slot>& slots, no::vector2f position) const {
	int i = 0;
	for (auto& slot : slots) {
		if (slot_transform(position, i).collides_with(game.ui_camera_2x.mouse_position(game.mouse()))) {
			return { i % 4, i / 4 };
		}
		i++;
	}
	return -1;
}	

void trading_view::add_item(inventory_container& items, std::vector<item_slot>& slots, item_instance item) {
	bool exists = false;
	for (auto& slot : slots) {
		if (slot.item.definition_id == item.definition_id) {
			exists = (item.definition().max_stack > slot.item.stack + item.stack);
			break;
		}
	}
	if (!exists) {
		bool has_empty = false;
		for (auto& slot : slots) {
			if (slot.item.definition_id == -1) {
				has_empty = true;
				slot.item = item;
				set_item_uv(slot.rectangle, item.definition().uv);
				break;
			}
		}
		if (!has_empty) {
			auto& slot = slots.emplace_back();
			slot.item = item;
			set_item_uv(slot.rectangle, item.definition().uv);
		}
	}
	items.add_from(item);
	other_accepted = false;
	waiting = false;
}

void trading_view::remove_item(inventory_container& items, std::vector<item_slot>& slots, no::vector2i slot) {
	int slot_index = slot.y * 4 + slot.x;
	if (slot_index < 0 || slot_index >= (int)slots.size()) {
		return;
	}
	items.items[slot_index] = {};
	slots[slot_index].item = {};
	other_accepted = false;
	waiting = false;
}

void trading_view::add_context_options(no::vector2f position, std::vector<item_slot>& slots, bool mine) {
	no::vector2i slot = hovered_slot(slots, position);
	if (slot.x == -1) {
		return;
	}
	int slot_index = slot.y * inventory_container::columns + slot.x;
	auto item = slots[slot_index].item;
	if (item.definition_id == -1) {
		return;
	}
	if (mine) {
		add_context_menu_option("Remove " + item.definition().name, [this, item, slot] {
			item_instance temp{ item };
			game.world.my_player().character.inventory.add_from(temp);
			game.send_remove_trade_item(slot);
		});
	}
	add_context_menu_option("Examine " + item.definition().name, [this, item] {
		add_chat_message("", std::to_string(item.stack) + "x " + item.definition().name);
	});
}

void trading_view::finish() {
	if (waiting && other_accepted) {
		auto& player = game.world.my_player().character;
		for (auto& item : left.items) {
			player.inventory.add_from(item);
		}
		stop_trading();
	}
}

static trading_view* trading = nullptr;

void start_trading(game_state& game, int trader_id) {
	stop_trading();
	trading = new trading_view{ game, trader_id };
}

bool is_trading() {
	return trading != nullptr;
}

void notify_trade_decision(bool accepted) {
	if (!trading) {
		return;
	}
	trading->other_accepted = accepted;
	trading->finish();
}

void add_trade_item(int which, item_instance item) {
	ASSERT(trading);
	if (which == 0) {
		trading->add_item(trading->left, trading->left_slots, item);
	} else if (which == 1) {
		trading->add_item(trading->right, trading->right_slots, item);
	}
}

void remove_trade_item(int which, no::vector2i slot) {
	ASSERT(trading);
	if (which == 0) {
		trading->remove_item(trading->left, trading->left_slots, slot);
	} else if (which == 1) {
		trading->remove_item(trading->right, trading->right_slots, slot);
	}
}

void add_trading_context_options() {
	if (!trading) {
		return;
	}
	no::vector2f position;
	position.x = trading->transform.position.x + 16.0f;
	position.y = trading->transform.position.y + 16.0f;
	trading->add_context_options(position, trading->left_slots, false);
	position.x = trading->transform.position.x + trading->transform.scale.x - item_grid.x * 4.0f - 16.0f;
	trading->add_context_options(position, trading->right_slots, true);
}

void update_trading_ui() {
	if (trading) {
		trading->update();
	}
}

void draw_trading_ui() {
	if (trading) {
		trading->draw();
	}
}

void stop_trading() {
	delete trading;
	trading = nullptr;
}
