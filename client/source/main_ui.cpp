#include "main_ui.hpp"
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

constexpr no::vector4f context_uv_top = { 121.0f, 58.0f, 191.0f - 121.0f, 67.0f - 58.0f };
constexpr no::vector4f context_uv_row = { 121.0f, 68.0f, 191.0f - 121.0f, 79.0f - 68.0f };
constexpr no::vector4f context_uv_bottom = { 121.0f, 111.0f, 191.0f - 121.0f, 116.0f - 111.0f };

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
	return option_transform(index).collides_with(camera.mouse_position(mouse));
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
	no::transform2 transform;
	// top
	transform.position = position;
	transform.scale = context_uv_top.zw;
	no::draw_shape(top, transform);

	// rows
	transform.position.y += context_uv_top.w;
	transform.scale = context_uv_row.zw;
	no::vector2f mouse_position = camera.mouse_position(mouse);
	for (auto& option : options) {
		no::draw_shape(row, transform);
		transform.position.y += transform.scale.y;
	}

	// bottom
	transform.scale = context_uv_bottom.zw;
	no::draw_shape(bottom, transform);

	// text
	transform.position.x += 16.0f;
	transform.position.y = position.y + context_uv_top.w;
	for (auto& option : options) {
		transform.scale = context_uv_row.zw;
		if (transform.collides_with(no::vector2f{ mouse_position.x + 16.0f, mouse_position.y })) {
			color.set({ 1.0f, 0.7f, 0.3f, 1.0f });
		}
		transform.scale = no::texture_size(option.texture).to<float>();
		no::bind_texture(option.texture);
		no::draw_shape(full, transform);
		transform.position.y += context_uv_row.w;
		color.set({ 1.0f, 1.0f, 1.0f, 1.0f });
	}
}

no::transform2 context_menu::option_transform(int index) const {
	no::transform2 transform;
	transform.scale = context_uv_row.zw;
	transform.position = position;
	transform.position.y += context_uv_top.w + transform.scale.y * (float)index;
	return transform;
}

no::transform2 context_menu::menu_transform() const {
	no::transform2 transform;
	transform.position = position;
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

void inventory_view::on_item_added(const item_container::add_event& event) {
	int i = event.slot.y * 4 + event.slot.x;
	auto slot = slots.find(i);
	if (slot == slots.end()) {
		slots[i] = {};
		slots[i].item = event.item;
		set_item_uv(slots[i].rectangle, item_definitions().get(event.item.definition_id).uv);
	} else {
		// todo: update stack text
	}
}

void inventory_view::on_item_removed(const item_container::remove_event& event) {
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
}

void inventory_view::listen(character_object* player_) {
	player = player_;
	add_item_event = player->inventory.events.add.listen([this](const item_container::add_event& event) {
		on_item_added(event);
	});
	remove_item_event = player->inventory.events.remove.listen([this](const item_container::remove_event& event) {
		on_item_removed(event);
	});
	player->inventory.for_each([this](no::vector2i slot, const item_instance& item) {
		on_item_added({ item, slot });
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

no::transform2 inventory_view::body_transform() const {
	no::transform2 transform;
	transform.scale = inventory_uv.zw;
	transform.position.x = camera.width() - background_uv.z - 2.0f + inventory_offset.x;
	transform.position.y = inventory_offset.y;
	return transform;
}

no::transform2 inventory_view::slot_transform(int index) const {
	no::transform2 transform;
	transform.scale = item_size;
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
		if (slot_transform(slot.first).collides_with(camera.mouse_position(game.mouse()))) {
			return { slot.first % 4, slot.first / 4 };
		}
	}
	return -1;
}

user_interface_view::user_interface_view(game_state& game, world_state& world) 
	: game(game), world(world), inventory(camera, game, world), font(no::asset_path("fonts/leo.ttf"), 9) {
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
	no::transform2 transform;
	transform.scale = background_uv.zw;
	transform.position.x = camera.width() - transform.scale.x - 2.0f;
	return transform.collides_with(camera.mouse_position(game.mouse()));
}

bool user_interface_view::is_mouse_over_context() const {
	return context && context->menu_transform().collides_with(camera.mouse_position(game.mouse()));
}

bool user_interface_view::is_mouse_over_inventory() const {
	return inventory.body_transform().collides_with(camera.mouse_position(game.mouse()));
}

bool user_interface_view::is_tab_hovered(int index) const {
	return tab_transform(index).collides_with(camera.mouse_position(game.mouse()));
}

bool user_interface_view::is_mouse_over_any() const {
	return is_mouse_over() || is_mouse_over_context();
}

void user_interface_view::listen(character_object* player_) {
	ASSERT(player_);
	player = player_;
	equipment_event = player->events.equip.listen([this](const character_object::equip_event& event) {
		
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
	no::transform2 transform;
	transform.scale = background_uv.zw;
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
	no::transform2 transform;
	transform.position = 8.0f;
	transform.scale = hud_size;
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

no::transform2 user_interface_view::tab_transform(int index) const {
	no::transform2 transform;
	transform.position = { 429.0f, 146.0f };
	transform.position -= background_uv.xy;
	transform.position.x += camera.width() - background_uv.z - 2.0f + (tab_size.x + 4.0f) * (float)index;
	transform.scale = tab_size;
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
					context->add_option("Equip", [this, slot] {
						game.equip_from_inventory(slot);
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
	} else {
		no::vector2i tile = game.hovered_tile();
		auto& objects = game.world.objects;
		objects.for_each([this, tile](game_object* object) {
			if (object->tile() != tile) {
				return;
			}
			auto& definition = object->definition();
			if (definition.type == game_object_type::character) {
				auto character = (character_object*)object;
				if (character->health.value() > 0) {
					int target_id = object->id(); // objects array can be resized
					context->add_option("Attack " + definition.name, [this, target_id] {
						game.start_combat(target_id);
					});
				}
			}
			if (definition.dialogue_id != -1) {
				std::string option_name = "Use ";
				if (definition.type == game_object_type::character) {
					option_name = "Talk to ";
				}
				context->add_option(option_name + definition.name, [&] {
					game.start_dialogue(definition.dialogue_id);
				});
			}
			if (!definition.description.empty()) {
				context->add_option("Examine " + definition.name, [&] {
					game.chat.add("", definition.description);
				});
			}
		});
	}
	if (context->count() == 0) {
		delete context;
		context = nullptr;
	}
}

hit_splat::hit_splat(game_state& game, int target_id, int value) : game(&game), target_id(target_id) {
	texture = no::create_texture(game.font().render(std::to_string(value)));
	background = no::create_texture({ 2, 2, no::pixel_format::rgba, 0xFF0000FF });
}

hit_splat::hit_splat(hit_splat&& that) {
	std::swap(game, that.game);
	std::swap(transform, that.transform);
	std::swap(target_id, that.target_id);
	std::swap(texture, that.texture);
	std::swap(fade_in, that.fade_in);
	std::swap(stay, that.stay);
	std::swap(fade_out, that.fade_out);
	std::swap(alpha, that.alpha);
	std::swap(background, that.background);
}

hit_splat::~hit_splat() {
	no::delete_texture(texture);
	no::delete_texture(background);
}

void hit_splat::update() {
	if (fade_in < 1.0f) {
		fade_in += 0.02f;
		alpha = fade_in;
	} else if (stay < 1.0f) {
		stay += 0.04f;
		alpha = 1.0f;
	} else if (fade_out < 1.0f) {
		fade_out += 0.03f;
		alpha = 1.0f - fade_out;
	}
	auto target = game->world.objects.find(target_id);
	if (target) {
		no::vector3f position = target->transform.position;
		position.y += 2.0f; // todo: height of model
		transform.position = game->world_camera().world_to_screen(position);
		transform.position.y -= (fade_in * 0.5f + fade_out) * 32.0f;
	}
	transform.scale = no::texture_size(texture).to<float>() * 2.0f;
}

void hit_splat::draw(no::shader_variable color, const no::rectangle& rectangle) const {
	auto background_transform = transform;
	background_transform.position -= 4.0f;
	background_transform.scale.x += 4.0f;
	no::bind_texture(background);
	color.set({ 1.0f, 1.0f, 1.0f, alpha * 0.75f });
	no::draw_shape(rectangle, background_transform);
	auto shadow_transform = transform;
	shadow_transform.position += 2.0f;
	no::bind_texture(texture);
	color.set({ 0.0f, 0.0f, 0.0f, alpha });
	no::draw_shape(rectangle, shadow_transform);
	color.set({ 1.0f, 1.0f, 1.0f, alpha });
	no::draw_shape(rectangle, transform);
}

bool hit_splat::is_visible() const {
	return fade_out < 1.0f;
}
