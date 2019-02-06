#include "ui.hpp"
#include "game.hpp"

#include "assets.hpp"
#include "surface.hpp"

constexpr no::vector2f ui_size = { 600.0f, 400.0f };
constexpr no::vector2f item_size = 32.0f;
constexpr no::vector2f item_grid = item_size + 2.0f;
constexpr no::vector4f background_uv = { 392.0f, 48.0f, 184.0f, 352.0f };
constexpr no::vector2f tab_background_uv = { 128.0f, 24.0f };
constexpr no::vector2f tab_inventory_uv = { 160.0f, 24.0f };
constexpr no::vector2f tab_equipment_uv = { 224.0f, 24.0f };
constexpr no::vector2f tab_quests_uv = { 192.0f, 24.0f };
constexpr no::vector2f tab_size = 24.0f;

static void set_ui_uv(no::rectangle& rectangle, no::vector2f uv, no::vector2f uv_size) {
	rectangle.set_tex_coords(uv.x / ui_size.x, uv.y / ui_size.y, uv_size.x / ui_size.x, uv_size.y / ui_size.y);
}

static void set_ui_uv(no::rectangle& rectangle, no::vector4f uv) {
	set_ui_uv(rectangle, uv.xy, uv.zw);
}

static void set_item_uv(no::rectangle& rectangle, no::vector2f uv) {
	set_ui_uv(rectangle, uv, item_size);
}

inventory_view::inventory_view(game_state& game, world_state& world) : game(game), world(world) {

}

inventory_view::~inventory_view() {
	ignore();
}

void inventory_view::listen(player_object* player_) {
	player = player_;
	add_item_event = player->inventory.events.add.listen([this](const item_container::add_event& event) {
		int i = event.slot.y * 4 + event.slot.x;
		auto slot = slots.find(i);
		if (slot == slots.end()) {
			slots[i] = {};
			set_item_uv(slots[i].rectangle, world.items().get(event.item.definition_id).uv);
		} else {
			// todo: update stack text
		}
	});
	remove_item_event = player->inventory.events.remove.listen([this](const item_container::remove_event& event) {
		int i = event.slot.y * 4 + event.slot.x;
		auto slot = slots.find(i);
		if (slot != slots.end()) {
			ASSERT(slot->second.item.definition_id == event.item.definition_id);
			slot->second.item.stack -= event.item.stack;
			if (slot->second.item.stack <= 0) {
				slots.erase(i);
			} else {
				// todo: update stack text
			}
		}
	});
}

void inventory_view::ignore() {
	if (!player) {
		return;
	}
	player->inventory.events.add.ignore(add_item_event);
	player->inventory.events.remove.ignore(remove_item_event);
	add_item_event = -1;
	remove_item_event = -1;
	player = nullptr;
}

void inventory_view::draw(const no::ortho_camera& camera) const {
	no::transform transform;
	transform.scale.xy = item_size;
	for (auto& slot : slots) {
		transform.position.x = camera.width() - background_uv.z + 20.0f + (float)(slot.first % 4) * item_grid.x;
		transform.position.y = 132.0f + (float)(slot.first / 4) * item_grid.y;
		no::set_shader_model(transform);
		slot.second.rectangle.bind();
		slot.second.rectangle.draw();
	}
}

user_interface_view::user_interface_view(game_state& game, world_state& world) 
	: game(game), world(world), inventory(game, world) {
	camera.zoom = 2.0f;
	shader = no::create_shader(no::asset_path("shaders/sprite"));
	color = no::get_shader_variable("uni_Color");
	ui_texture = no::create_texture(no::surface(no::asset_path("sprites/ui.png")));
	set_ui_uv(background, background_uv);
	set_ui_uv(tabs.background, tab_background_uv, tab_size);
	set_ui_uv(tabs.inventory, tab_inventory_uv, tab_size);
	set_ui_uv(tabs.equipment, tab_equipment_uv, tab_size);
	set_ui_uv(tabs.quests, tab_quests_uv, tab_size);
}

user_interface_view::~user_interface_view() {
	ignore();
	no::delete_texture(ui_texture);
}

void user_interface_view::listen(player_object* player_) {
	ASSERT(player_);
	player = player_;
	equipment_event = player->events.equip.listen([this](const player_object::equip_event& event) {
		if (event.item_id == -1) {

		} else {

		}
	});
	press_event_id = game.mouse().press.listen([this](const no::mouse::press_message& event) {
		if (event.button != no::mouse::button::left) {
			return;
		}
		for (int i = 0; i < 4; i++) {
			if (is_tab_hovered(i)) {
				tabs.active = i;
				break;
			}
		}
	});
	inventory.listen(player);
}

void user_interface_view::ignore() {
	if (!player) {
		return;
	}
	inventory.ignore();
	player->events.equip.ignore(equipment_event);
	equipment_event = -1;
	game.mouse().press.ignore(press_event_id);
	press_event_id = -1;
	player = nullptr;
}

void user_interface_view::draw() const {
	no::bind_shader(shader);
	no::set_shader_view_projection(camera);
	color.set(no::vector4f{ 1.0f });
	no::bind_texture(ui_texture);
	no::transform transform;
	transform.scale.xy = background_uv.zw;
	transform.position.x = camera.width() - transform.scale.x - 2.0f;
	no::set_shader_model(transform);
	background.bind();
	background.draw();
	switch (tabs.active) {
	case 0:
		inventory.draw(camera);
		break;
	case 1:
		break;
	case 2:
		break;
	case 3:
		break;
	}
	draw_tabs();
}

void user_interface_view::draw_tabs() const {
	draw_tab(0, tabs.inventory);
	draw_tab(1, tabs.equipment);
	draw_tab(2, tabs.quests);
}

void user_interface_view::draw_tab(int index, const no::rectangle& tab) const {
	no::set_shader_model(tab_transform(index));
	bool hover = is_tab_hovered(index);
	if (hover) {
		color.set({ 1.0f, 0.7f, 0.3f, 1.0f });
	}
	if (tabs.active == index) {
		color.set({ 1.0f, 0.2f, 0.2f, 1.0f });
	}
	tabs.background.bind();
	tabs.background.draw();
	color.set(no::vector4f{ 1.0f });
	tab.bind();
	tab.draw();
}

no::transform user_interface_view::tab_transform(int index) const {
	no::transform transform;
	transform.position.xy = { 429.0f, 146.0f };
	transform.position.xy -= background_uv.xy;
	transform.position.x += camera.width() - background_uv.z - 2.0f + (tab_size.x + 4.0f) * (float)index;
	transform.scale.xy = tab_size;
	return transform;
}

bool user_interface_view::is_tab_hovered(int index) const {
	no::vector2f mouse = camera.mouse_position(game.mouse());
	return tab_transform(index).collides_with(no::vector3f{ mouse.x, mouse.y, 0.0f });
}
