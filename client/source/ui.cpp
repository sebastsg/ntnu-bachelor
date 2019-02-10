#include "ui.hpp"
#include "game.hpp"

#include "assets.hpp"
#include "surface.hpp"

constexpr no::vector2f ui_size = 1024.0f;
constexpr no::vector2f item_size = 32.0f;
constexpr no::vector2f item_grid = item_size + 2.0f;
constexpr no::vector4f background_uv = { 391.0f, 48.0f, 184.0f, 352.0f };
constexpr no::vector2f tab_background_uv = { 128.0f, 24.0f };
constexpr no::vector2f tab_inventory_uv = { 160.0f, 24.0f };
constexpr no::vector2f tab_equipment_uv = { 224.0f, 24.0f };
constexpr no::vector2f tab_quests_uv = { 192.0f, 24.0f };
constexpr no::vector2f tab_size = 24.0f;
constexpr no::vector2f hud_uv = { 108.0f, 128.0f };
constexpr no::vector2f hud_size = { 88.0f, 68.0f };
constexpr no::vector4f inventory_uv = { 200.0f, 128.0f, 138.0f, 205.0f };
constexpr no::vector2f inventory_offset = { 23.0f, 130.0f };

constexpr no::vector4f context_uv_top = { 12.0f, 12.0f, 88.0f, 12.0f };
constexpr no::vector4f context_uv_row = { 12.0f, 24.0f, 88.0f, 8.0f };
constexpr no::vector4f context_uv_bottom = { 12.0f, 40.0f, 88.0f, 12.0f };

static void set_ui_uv(no::rectangle& rectangle, no::vector2f uv, no::vector2f uv_size) {
	rectangle.set_tex_coords(uv.x / ui_size.x, uv.y / ui_size.y, uv_size.x / ui_size.x, uv_size.y / ui_size.y);
}

static void set_ui_uv(no::rectangle& rectangle, no::vector4f uv) {
	set_ui_uv(rectangle, uv.xy, uv.zw);
}

static void set_item_uv(no::rectangle& rectangle, no::vector2f uv) {
	set_ui_uv(rectangle, uv, item_size);
}

context_menu::context_menu(const no::ortho_camera& camera_, no::vector2f position_, const no::font& font, no::mouse& mouse_)
	: camera(camera_), mouse(mouse_), position(position_), font(font) {
	set_ui_uv(top, context_uv_top);
	set_ui_uv(row, context_uv_row);
	set_ui_uv(bottom, context_uv_bottom);
}

context_menu::~context_menu() {
	for (auto& option : options) {
		no::delete_texture(option.texture);
	}
}

void context_menu::trigger(int index) {
	if (options[index].action) {
		options[index].action();
	}
}

bool context_menu::is_mouse_over(int index) const {
	no::vector2f mouse_position = camera.mouse_position(mouse);
	return option_transform(index).collides_with(no::vector3f{ mouse_position.x, mouse_position.y, 0.0f });
}

int context_menu::count() const {
	return (int)options.size();
}

void context_menu::add_option(const std::string& text, const std::function<void()>& action) {
	auto& option = options.emplace_back();
	option.text = text;
	option.action = action;
	option.texture = no::create_texture(font.render(text));
	max_width = context_uv_top.z;
}

void context_menu::draw() const {
	if (!color.exists()) {
		color = no::get_shader_variable("uni_Color");
	}
	no::transform transform;
	// top
	transform.position.xy = position;
	transform.scale.xy = context_uv_top.zw;
	no::draw_shape(top, transform);

	// rows
	transform.position.y += context_uv_top.w;
	transform.scale.xy = context_uv_row.zw;
	no::vector2f mouse_position = camera.mouse_position(mouse);
	for (auto& option : options) {
		no::draw_shape(row, transform);
		transform.position.y += transform.scale.y;
	}

	// bottom
	transform.scale.xy = context_uv_bottom.zw;
	no::draw_shape(bottom, transform);

	// text
	transform.position.x += 16.0f;
	transform.position.y = position.y + context_uv_top.w;
	for (auto& option : options) {
		transform.scale.xy = context_uv_row.zw;
		if (transform.collides_with(no::vector3f{ mouse_position.x + 16.0f, mouse_position.y, 0.0f })) {
			color.set({ 1.0f, 0.7f, 0.3f, 1.0f });
		}
		transform.scale.xy = no::texture_size(option.texture).to<float>();
		no::bind_texture(option.texture);
		no::draw_shape(full, transform);
		transform.position.y += context_uv_row.w;
		color.set({ 1.0f, 1.0f, 1.0f, 1.0f });
	}
}

no::transform context_menu::option_transform(int index) const {
	no::transform transform;
	transform.scale.xy = context_uv_row.zw;
	transform.position.xy = position;
	transform.position.y += context_uv_top.w + transform.scale.y * (float)index;
	return transform;
}

no::transform context_menu::menu_transform() const {
	no::transform transform;
	transform.position.xy = position;
	transform.scale.x = max_width;
	transform.scale.y = context_uv_top.w + context_uv_row.w * (float)options.size() + context_uv_bottom.w;
	return transform;
}

inventory_view::inventory_view(const no::ortho_camera& camera, game_state& game, world_state& world) 
	: camera(camera), game(game), world(world) {
	set_ui_uv(background, inventory_uv);
}

inventory_view::~inventory_view() {
	ignore();
}

void inventory_view::listen(character_object* player_) {
	player = player_;
	add_item_event = player->inventory.events.add.listen([this](const item_container::add_event& event) {
		int i = event.slot.y * 4 + event.slot.x;
		auto slot = slots.find(i);
		if (slot == slots.end()) {
			slots[i] = {};
			slots[i].item = event.item;
			set_item_uv(slots[i].rectangle, item_definitions().get(event.item.definition_id).uv);
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

no::transform inventory_view::body_transform() const {
	no::transform transform;
	transform.scale.xy = inventory_uv.zw;
	transform.position.x = camera.width() - background_uv.z - 2.0f + inventory_offset.x;
	transform.position.y = inventory_offset.y;
	return transform;
}

no::transform inventory_view::slot_transform(int index) const {
	no::transform transform;
	transform.scale.xy = item_size;
	transform.position.x = camera.width() - background_uv.z + 23.0f + (float)(index % 4) * item_grid.x;
	transform.position.y = 132.0f + (float)(index / 4) * item_grid.y;
	return transform;
}

void inventory_view::draw() const {
	no::draw_shape(background, body_transform());
	for (auto& slot : slots) {
		no::draw_shape(slot.second.rectangle, slot_transform(slot.first));
	}
}

no::vector2i inventory_view::hovered_slot() const {
	for (auto& slot : slots) {
		no::vector2f mouse = camera.mouse_position(game.mouse());
		if (slot_transform(slot.first).collides_with(no::vector3f{ mouse.x, mouse.y, 0.0f })) {
			return { slot.first % 4, slot.first / 4 };
		}
	}
	return -1;
}

user_interface_view::user_interface_view(game_state& game, world_state& world) 
	: game(game), world(world), inventory(camera, game, world), font(no::asset_path("fonts/leo.ttf"), 6) {
	camera.zoom = 2.0f;
	shader = no::create_shader(no::asset_path("shaders/sprite"));
	color = no::get_shader_variable("uni_Color");
	ui_texture = no::create_texture(no::surface(no::asset_path("sprites/ui.png")));
	set_ui_uv(background, background_uv);
	set_ui_uv(tabs.background, tab_background_uv, tab_size);
	set_ui_uv(tabs.inventory, tab_inventory_uv, tab_size);
	set_ui_uv(tabs.equipment, tab_equipment_uv, tab_size);
	set_ui_uv(tabs.quests, tab_quests_uv, tab_size);
	set_ui_uv(hud_background, hud_uv, hud_size);
}

user_interface_view::~user_interface_view() {
	ignore();
	no::delete_texture(ui_texture);
	delete context;
}

bool user_interface_view::is_mouse_over() const {
	no::transform transform;
	transform.scale.xy = background_uv.zw;
	transform.position.x = camera.width() - transform.scale.x - 2.0f;
	no::vector2f mouse = camera.mouse_position(game.mouse());
	return transform.collides_with(no::vector3f{ mouse.x, mouse.y, 0.0f });
}

bool user_interface_view::is_mouse_over_context() const {
	if (!context) {
		return false;
	}
	no::vector2f mouse = camera.mouse_position(game.mouse());
	return context->menu_transform().collides_with(no::vector3f{ mouse.x, mouse.y, 0.0f });
}

bool user_interface_view::is_mouse_over_inventory() const {
	no::vector2f mouse = camera.mouse_position(game.mouse());
	return inventory.body_transform().collides_with(no::vector3f{ mouse.x, mouse.y, 0.0f });
}

bool user_interface_view::is_tab_hovered(int index) const {
	no::vector2f mouse = camera.mouse_position(game.mouse());
	return tab_transform(index).collides_with(no::vector3f{ mouse.x, mouse.y, 0.0f });
}

bool user_interface_view::is_mouse_over_any() const {
	return is_mouse_over() || is_mouse_over_context();
}

void user_interface_view::listen(character_object* player_) {
	ASSERT(player_);
	player = player_;
	equipment_event = player->events.equip.listen([this](const character_object::equip_event& event) {
		if (event.item_id == -1) {

		} else {

		}
	});
	press_event_id = game.mouse().press.listen([this](const no::mouse::press_message& event) {
		if (event.button == no::mouse::button::left) {
			if (context) {
				for (int i = 0; i < context->count(); i++) {
					if (context->is_mouse_over(i)) {
						context->trigger(i);
						delete context;
						context = nullptr;
						return; // don't let through tab click
					}
				}
			}
			delete context;
			context = nullptr;
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

void user_interface_view::update() {
	
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
		inventory.draw();
		break;
	case 1:
		break;
	case 2:
		break;
	case 3:
		break;
	}
	draw_tabs();
	draw_hud();
	if (context) {
		context->draw();
	}
}

void user_interface_view::draw_hud() const {
	no::transform transform;
	transform.position.xy = 8.0f;
	transform.scale.xy = hud_size;
	no::set_shader_model(transform);
	hud_background.bind();
	hud_background.draw();
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

void user_interface_view::create_context() {
	delete context;
	context = new context_menu(camera, camera.mouse_position(game.mouse()), font, game.mouse());
	if (is_mouse_over_inventory()) {
		no::vector2i slot = inventory.hovered_slot();
		if (slot.x != -1) {
			auto item = player->inventory.at(slot);
			if (item.definition_id != -1) {
				auto definition = item_definitions().get(item.definition_id);
				context->add_option("Use", [] {

				});
				if (definition.type == item_type::equipment) {
					context->add_option("Equip", [this, definition, item, slot] {
						player->events.equip.emit({ definition.slot, item.definition_id });
					});
				}
				context->add_option("Drop", [this, item, slot] {
					item_instance ground_item;
					ground_item.definition_id = item.definition_id;
					player->inventory.remove_to(item.stack, ground_item);
					// todo: drop on ground
				});
			}
		}
	}
}
