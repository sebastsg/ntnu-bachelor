#include "ui.hpp"

#include "assets.hpp"
#include "surface.hpp"

user_interface_view::user_interface_view(world_state& world) : world(world) {
	shader = no::create_shader(no::asset_path("shaders/sprite"));
	ui_texture = no::create_texture(no::surface(no::asset_path("sprites/ui.png")));
	set_ui_uv(inventory.background, inventory_uv);
	camera.zoom = 2.0f;
}

user_interface_view::~user_interface_view() {
	ignore();
	no::delete_texture(ui_texture);
}

void user_interface_view::set_ui_uv(no::rectangle& rectangle, no::vector4f uv) {
	const auto size = no::texture_size(ui_texture).to<float>();
	rectangle.set_tex_coords(uv.x / size.x, uv.y / size.y, uv.z / size.x, uv.w / size.y);
}

void user_interface_view::set_item_uv(no::rectangle& rectangle, no::vector2f uv) {
	set_ui_uv(rectangle, { uv.x, uv.y, item_size.x, item_size.y });
}

void user_interface_view::listen(player_object* player_) {
	ASSERT(player_);
	player = player_;
	equipment_event = player->events.equip.listen([this](const player_object::equip_event& event) {
		if (event.item_id == -1) {

		} else {

		}
	});
	inventory.add_item_event = player->inventory.events.add.listen([this](const item_container::add_event& event) {
		int i = event.slot.y * 4 + event.slot.x;
		auto slot = inventory.slots.find(i);
		if (slot == inventory.slots.end()) {
			inventory.slots[i] = {};
			set_item_uv(inventory.slots[i].rectangle, world.items().get(event.item.definition_id).uv);
		} else {
			// todo: update stack text
		}
	});
	inventory.remove_item_event = player->inventory.events.remove.listen([this](const item_container::remove_event& event) {
		int i = event.slot.y * 4 + event.slot.x;
		auto slot = inventory.slots.find(i);
		if (slot != inventory.slots.end()) {
			ASSERT(slot->second.item.definition_id == event.item.definition_id);
			slot->second.item.stack -= event.item.stack;
			if (slot->second.item.stack <= 0) {
				inventory.slots.erase(i);
			} else {
				// todo: update stack text
			}
		}
	});
}

void user_interface_view::ignore() {
	if (!player) {
		return;
	}
	player->events.equip.ignore(equipment_event);
	player->inventory.events.add.ignore(inventory.add_item_event);
	player->inventory.events.remove.ignore(inventory.remove_item_event);
	equipment_event = -1;
	inventory = {};
	player = nullptr;
}

void user_interface_view::draw() const {
	no::bind_shader(shader);
	no::set_shader_view_projection(camera);
	no::get_shader_variable("uni_Color").set({ 1.0f, 1.0f, 1.0f, 1.0f });
	no::bind_texture(ui_texture);
	no::transform transform;
	transform.scale.xy = inventory_uv.zw;
	transform.position.x = camera.width() - transform.scale.x - 2.0f;
	no::set_shader_model(transform);
	inventory.background.bind();
	inventory.background.draw();
	transform.scale.xy = item_size;
	for (auto& slot : inventory.slots) {
		transform.position.x = camera.width() - inventory_uv.z + 20.0f + (float)(slot.first % 4) * item_grid.x;
		transform.position.y = 132.0f + (float)(slot.first / 4) * item_grid.y;
		no::set_shader_model(transform);
		slot.second.rectangle.bind();
		slot.second.rectangle.draw();
	}
}
