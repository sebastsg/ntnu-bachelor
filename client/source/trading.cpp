#include "trading.hpp"
#include "game.hpp"
#include "ui_context.hpp"

const no::vector4f trade_background_uv{ 8.0f, 632.0f, 352.0f, 240.0f };
const no::vector4f button_uv{ 8.0f, 992.0f, 264.0f, 24.0f };

struct trading_view {

	game_state& game;
	int trader_id = -1;

	no::rectangle background;
	no::button accept;
	no::button cancel;
	no::transform2 transform;
	no::font font;

	no::rectangle rectangle;
	no::text_view waiting_label;
	bool waiting = false;
	no::text_view other_accepted_label;
	bool other_accepted = false;

	inventory_container left;
	inventory_container right;
	std::vector<item_slot> left_slots;
	std::vector<item_slot> right_slots;

	trading_view(game_state& game, int trader_id);

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
	accept{ game_, game_.ui_camera }, 
	cancel{ game_, game_.ui_camera },
	font{ no::asset_path("fonts/leo.ttf"), 10 }, 
	other_accepted_label{ game_, game_.ui_camera }, 
	waiting_label{ game_, game_.ui_camera } {
	set_ui_uv(background, trade_background_uv);
	transform.scale = trade_background_uv.zw;
	accept.set_tex_coords(button_uv.xy / 1024.0f, button_uv.zw / 1024.0f);
	cancel.set_tex_coords(button_uv.xy / 1024.0f, button_uv.zw / 1024.0f);
	accept.transform.scale = button_uv.zw;
	cancel.transform.scale = button_uv.zw;
	accept.transform.scale.x /= 3.0f;
	cancel.transform.scale.x /= 3.0f;
	accept.label.render(font, "Accept");
	cancel.label.render(font, "Cancel");
	accept.events.click.listen([this] {
		game.send_finish_trading(true);
		waiting = true;
		finish();
	});
	cancel.events.click.listen([this] {
		game.send_finish_trading(false);
	});
	other_accepted_label.render(font, "Other player has accepted this trade");
	waiting_label.render(font, "Waiting for other player...");
}

void trading_view::update() {
	transform.position = game.ui_camera.size() / 2.0f - transform.scale / 2.0f;
	accept.transform.position.x = transform.position.x + transform.scale.x / 2.0f - 4.0f - 88.0f;
	cancel.transform.position.x = transform.position.x + transform.scale.x / 2.0f + 4.0f;
	accept.transform.position.y = transform.position.y + transform.scale.y - accept.transform.scale.y - 16.0f;
	cancel.transform.position.y = transform.position.y + transform.scale.y - cancel.transform.scale.y - 16.0f;
	if (!waiting) {
		accept.update();
	}
	cancel.update();
	other_accepted_label.transform.position.x = transform.position.x + transform.scale.x / 2.0f - other_accepted_label.transform.scale.x / 2.0f;
	other_accepted_label.transform.position.y = transform.position.y + transform.scale.y - 48.0f;
	waiting_label.transform.position.x = transform.position.x + transform.scale.x / 2.0f - waiting_label.transform.scale.x / 2.0f;
	waiting_label.transform.position.y = transform.position.y + transform.scale.y - 48.0f;
}

void trading_view::draw() {
	accept.color = no::get_shader_variable("uni_Color");
	cancel.color = no::get_shader_variable("uni_Color");
	no::draw_shape(background, transform);
	int index = 0;
	no::vector2f position = transform.position + 16.0f;
	for (auto& slot : left_slots) {
		if (slot.item.definition_id != -1) {
			no::draw_shape(slot.rectangle, slot_transform(position, index));
		}
		index++;
	}
	index = 0;
	position.x = transform.position.x + transform.scale.x - item_grid.x * 4.0f - 16.0f;
	for (auto& slot : right_slots) {
		if (slot.item.definition_id != -1) {
			no::draw_shape(slot.rectangle, slot_transform(position, index));
		}
		index++;
	}
	if (!waiting) {
		accept.draw_button();
	}
	cancel.draw_button();
	if (!waiting) {
		accept.draw_label();
	}
	cancel.draw_label();
	no::get_shader_variable("uni_Color").set(no::vector4f{ 1.0f });
	if (other_accepted) {
		other_accepted_label.draw(rectangle);
	} else if (waiting) {
		waiting_label.draw(rectangle);
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
		if (slot_transform(position, i).collides_with(game.ui_camera.mouse_position(game.mouse()))) {
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
		game.chat.add("", std::to_string(item.stack) + "x " + item.definition().name);
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
