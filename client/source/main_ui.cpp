#include "main_ui.hpp"
#include "game.hpp"
#include "trading.hpp"

#include "assets.hpp"
#include "surface.hpp"

const no::vector2f ui_size = 1024.0f;
const no::vector2f item_size = 32.0f;
const no::vector2f item_grid = item_size + 2.0f;
const no::vector4f background_uv = { 391.0f, 48.0f, 184.0f, 352.0f };
const no::vector2f tab_background_uv = { 128.0f, 24.0f };
const no::vector2f tab_inventory_uv = { 160.0f, 24.0f };
const no::vector2f tab_equipment_uv = { 224.0f, 24.0f };
const no::vector2f tab_quests_uv = { 192.0f, 24.0f };
const no::vector2f tab_size = 24.0f;

const no::vector4f hud_left_uv = { 8.0f, 528.0f, 40.0f, 68.0f };
const no::vector4f hud_tile_1_uv = { 56.0f, 528.0f, 16.0f, 68.0f };
const no::vector4f hud_tile_2_uv = { 80.0f, 528.0f, 16.0f, 68.0f };
const no::vector4f hud_right_uv = { 104.0f, 528.0f, 16.0f, 68.0f };
const no::vector2f hud_health_background = { 104.0f, 152.0f };
const no::vector2f hud_health_foreground = { 112.0f, 152.0f };
const no::vector2f hud_health_size = { 8.0f, 12.0f };

const no::vector4f inventory_uv = { 200.0f, 128.0f, 138.0f, 205.0f };
const no::vector2f inventory_offset = { 23.0f, 130.0f };

const no::vector4f equipment_uv = { 199.0f, 336.0f, 138.0f, 205.0f };

const no::vector4f context_uv[] = {
	{ 60.0f, 57.0f, 12.0f, 15.0f }, // top begin
	{ 80.0f, 57.0f, 16.0f, 9.0f }, // top tile 1
	{ 104.0f, 57.0f, 24.0f, 9.0f }, // top tile 2
	{ 136.0f, 57.0f, 8.0f, 9.0f }, // top tile 3
	{ 152.0f, 57.0f, 8.0f, 15.0f }, // top end
	{ 88.0f, 96.0f, 10.0f, 16.0f }, // highlight begin
	{ 104.0f, 96.0f, 8.0f, 16.0f }, // highlight tile
	{ 120.0f, 96.0f, 10.0f, 16.0f }, // highlight end
	{ 104.0f, 80.0f, 8.0f, 16.0f }, // item tile
	{ 60.0f, 80.0f, 4.0f, 16.0f }, // left 1
	{ 60.0f, 104.0f, 4.0f, 16.0f }, // left 2
	{ 152.0f, 80.0f, 8.0f, 16.0f }, // right 1
	{ 152.0f, 104.0f, 8.0f, 16.0f }, // right 2
	{ 60.0f, 128.0f, 12.0f, 11.0f }, // bottom begin
	{ 80.0f, 128.0f, 16.0f, 10.0f }, // bottom tile 1
	{ 104.0f, 128.0f, 24.0f, 10.0f }, // bottom tile 2
	{ 136.0f, 128.0f, 8.0f, 10.0f }, // bottom tile 3
	{ 152.0f, 128.0f, 8.0f, 11.0f }, // bottom end
};

void set_ui_uv(no::rectangle& rectangle, no::vector2f uv, no::vector2f uv_size) {
	rectangle.set_tex_coords(uv.x / ui_size.x, uv.y / ui_size.y, uv_size.x / ui_size.x, uv_size.y / ui_size.y);
}

void set_ui_uv(no::rectangle& rectangle, no::vector4f uv) {
	set_ui_uv(rectangle, uv.xy, uv.zw);
}

void set_item_uv(no::rectangle& rectangle, no::vector2f uv) {
	set_ui_uv(rectangle, uv, item_size);
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

hit_splat& hit_splat::operator=(hit_splat&& that) {
	std::swap(game, that.game);
	std::swap(transform, that.transform);
	std::swap(target_id, that.target_id);
	std::swap(texture, that.texture);
	std::swap(fade_in, that.fade_in);
	std::swap(stay, that.stay);
	std::swap(fade_out, that.fade_out);
	std::swap(alpha, that.alpha);
	std::swap(background, that.background);
	return *this;
}

void hit_splat::update(const no::ortho_camera& camera) {
	if (wait < 1.0f) {
		wait += 0.04f;
		alpha = 0.0f;
	} else if (fade_in < 1.0f) {
		fade_in += 0.02f;
		alpha = fade_in;
	} else if (stay < 1.0f) {
		stay += 0.04f;
		alpha = 1.0f;
	} else if (fade_out < 1.0f) {
		fade_out += 0.03f;
		alpha = 1.0f - fade_out;
	}
	if (target_id != -1) {
		auto& target = game->world.objects.object(target_id);
		no::vector3f position = target.transform.position;
		position.y += 2.0f; // todo: height of model
		transform.position = game->world_camera().world_to_screen(position) / camera.zoom;
		transform.position.y -= (fade_in * 0.5f + fade_out) * 32.0f;
	}
	transform.scale = no::texture_size(texture).to<float>();
}

void hit_splat::draw(no::shader_variable color, const no::rectangle& rectangle) const {
	auto background_transform = transform;
	background_transform.position -= 2.0f;
	background_transform.scale.x += 2.0f;
	no::bind_texture(background);
	color.set({ 1.0f, 1.0f, 1.0f, alpha * 0.75f });
	no::draw_shape(rectangle, background_transform);
	auto shadow_transform = transform;
	shadow_transform.position += 1.0f;
	no::bind_texture(texture);
	color.set({ 0.0f, 0.0f, 0.0f, alpha });
	no::draw_shape(rectangle, shadow_transform);
	color.set({ 1.0f, 1.0f, 1.0f, alpha });
	no::draw_shape(rectangle, transform);
}

bool hit_splat::is_visible() const {
	return fade_out < 1.0f;
}

context_menu::context_menu(const no::ortho_camera& camera_, no::vector2f position_, const no::font& font, no::mouse& mouse_)
	: camera(camera_), mouse(mouse_), position(position_), font(font) {
	position.floor();
	for (int i = 0; i < total_tiles; i++) {
		set_ui_uv(rectangles.emplace_back(), context_uv[i]);
	}
}

context_menu::~context_menu() {
	for (auto& option : options) {
		no::delete_texture(option.texture);
	}
}

void context_menu::add_option(const std::string& text, const std::function<void()>& action) {
	auto& option = options.emplace_back();
	option.text = text;
	option.action = action;
	option.texture = no::create_texture(font.render(text));
	max_width = std::max(max_width, (float)no::texture_size(option.texture).x + context_uv[top_begin].z);
}

void context_menu::trigger(int index) {
	if (options[index].action) {
		options[index].action();
	}
}

bool context_menu::is_mouse_over(int index) const {
	return option_transform(index).collides_with(camera.mouse_position(mouse));
}

void context_menu::draw(int ui_texture) const {
	if (!color.exists()) {
		color = no::get_shader_variable("uni_Color");
	}
	draw_background();
	draw_top_border();
	draw_options(ui_texture);
	draw_side_borders();
	draw_bottom_border();
}

int context_menu::count() const {
	return (int)options.size();
}

float context_menu::calculate_max_offset_x() const {
	float x = context_uv[top_begin].z;
	while (max_width > x) {
		x += context_uv[top_tile_1].z;
		if (max_width > x) {
			x += context_uv[top_tile_3].z;
		}
	}
	x += context_uv[top_tile_2].z;
	return x;
}

no::transform2 context_menu::menu_transform() const {
	no::transform2 transform;
	transform.position = position;
	transform.scale = {
		calculate_max_offset_x(),
		context_uv[top_begin].w + context_uv[item_tile].w * (float)options.size() + context_uv[bottom_begin].w
	};
	return transform;
}

no::transform2 context_menu::option_transform(int index) const {
	no::transform2 transform;
	transform.position = position;
	transform.position.x += context_uv[left_1].z;
	transform.position.y += context_uv[top_begin].w - 8.0f + (float)index * context_uv[item_tile].w;
	transform.scale = { calculate_max_offset_x(), context_uv[item_tile].w };
	return transform;
}

void context_menu::draw_border(int uv, no::vector2f offset) const {
	no::draw_shape(rectangles[uv], no::transform2{ { position + offset }, context_uv[uv].zw });
}

void context_menu::draw_top_border() const {
	no::vector2f offset;
	draw_border(top_begin, offset);
	offset.x += context_uv[top_begin].z;
	while (max_width > offset.x) {
		draw_border(top_tile_1, offset);
		offset.x += context_uv[top_tile_1].z;
		if (max_width > offset.x) {
			draw_border(top_tile_3, offset);
			offset.x += context_uv[top_tile_3].z;
		}
	}
	draw_border(top_tile_2, offset);
	offset.x += context_uv[top_tile_2].z;
	draw_border(top_end, offset);
}

void context_menu::draw_background() const {
	no::transform2 transform;
	transform.position = position + no::vector2f{ context_uv[left_1].z, context_uv[top_tile_1].w };
	float height = context_uv[top_begin].w + (float)(options.size() - 1) * context_uv[item_tile].w;
	transform.scale = { calculate_max_offset_x(), height };
	no::draw_shape(rectangles[item_tile], transform);
}

void context_menu::draw_side_borders() const {
	no::vector2f offset;
	offset.y = context_uv[top_begin].w;
	float top_before_end_x = calculate_max_offset_x();
	for (int i = 0; i < (int)options.size() - 1; i++) {
		offset.x = 0.0f;
		draw_border(left_1 + i % 1, offset);
		draw_border(right_1 + i % 1, { top_before_end_x, offset.y });
		offset.y += context_uv[right_1].z;
	}
}

void context_menu::draw_bottom_border() const {
	no::vector2f offset;
	offset.y = context_uv[top_begin].w + (float)(options.size() - 1) * context_uv[item_tile].w;
	draw_border(bottom_begin, offset);
	offset.x = context_uv[bottom_begin].z;
	while (max_width > offset.x) {
		draw_border(bottom_tile_1, offset);
		offset.x += context_uv[bottom_tile_1].z;
		if (max_width > offset.x) {
			draw_border(bottom_tile_3, offset);
			offset.x += context_uv[bottom_tile_3].z;
		}
	}
	draw_border(bottom_tile_2, offset);
	offset.x += context_uv[bottom_tile_2].z;
	draw_border(bottom_end, offset);
}

void context_menu::draw_options(int ui_texture) const {
	for (int i = 0; i < (int)options.size(); i++) {
		draw_option(i, ui_texture);
	}
	no::bind_texture(ui_texture);
}

void context_menu::draw_option(int option_index, int ui_texture) const {
	no::vector2f offset;
	offset.x = context_uv[left_1].z;
	offset.y = context_uv[top_tile_1].w - 2.0f; // two padding pixels
	auto& option = options[option_index];
	if (is_mouse_over(option_index)) {
		draw_highlighted_option(option_index, ui_texture);
	}
	no::transform2 transform;
	transform.position = position + context_uv[top_tile_1].zw;
	transform.position.x -= 6.0f;
	transform.position.y += (float)option_index * context_uv[item_tile].w + 2.0f;
	transform.scale = no::texture_size(option.texture).to<float>();
	no::bind_texture(option.texture);
	no::draw_shape(full, transform);
	offset.y += context_uv[item_tile].w;
	color.set(no::vector4f{ 1.0f });
}

void context_menu::draw_highlighted_option(int option, int ui_texture) const {
	no::vector2f offset;
	offset.x = context_uv[left_1].z;
	offset.y = context_uv[top_tile_1].w + (float)option * context_uv[item_tile].w - 2.0f;
	no::bind_texture(ui_texture);
	draw_border(highlight_begin, offset);
	offset.x += context_uv[highlight_begin].z;
	float max_offset_x = calculate_max_offset_x();
	while (max_offset_x > offset.x + 8.0f) {
		draw_border(highlight_tile, offset);
		offset.x += context_uv[highlight_tile].z;
	}
	draw_border(highlight_end, offset);
	color.set({ 1.0f, 0.7f, 0.3f, 1.0f });
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
		set_item_uv(slots[slot_index].rectangle, item.definition().uv);
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

void inventory_view::add_context_options(context_menu& context) {
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
		context.add_option("Offer " + item.definition().name, [this, item, slot, player] {
			game.send_add_trade_item(item);
			player->inventory.items[slot.y * inventory_container::columns + slot.x] = {};
			player->inventory.events.change.emit(slot);
		});
	} else {
		if (item.definition().type == item_type::equipment) {
			context.add_option("Equip " + item.definition().name, [this, slot] {
				game.equip_from_inventory(slot);
			});
		}
		context.add_option("Drop " + item.definition().name, [this, item, slot, player] {
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
		set_item_uv(slots[slot].rectangle, item.definition().uv);
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

void equipment_view::add_context_options(context_menu& context) {
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
	context.add_option("Unequip", [this, slot] {
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

hud_view::hud_view() : font(no::asset_path("fonts/leo.ttf"), 10) {
	fps_texture = no::create_texture();
	debug_texture = no::create_texture();
	set_ui_uv(hud_left, hud_left_uv);
	set_ui_uv(hud_tile_1, hud_tile_1_uv);
	set_ui_uv(hud_tile_2, hud_tile_2_uv);
	set_ui_uv(hud_right, hud_right_uv);
	set_ui_uv(health_background, hud_health_background, hud_health_size);
	set_ui_uv(health_foreground, hud_health_foreground, hud_health_size);
}

void hud_view::update(const no::ortho_camera& camera) {
	for (int i = 0; i < (int)hit_splats.size(); i++) {
		hit_splats[i].update(camera);
		if (!hit_splats[i].is_visible()) {
			hit_splats.erase(hit_splats.begin() + i);
			i--;
		}
	}
}

void hud_view::draw(no::shader_variable color, int ui_texture, character_object* player) const {
	color.set(no::vector4f{ 1.0f });
	no::transform2 transform;
	transform.position = { 300.0f, 4.0f };
	transform.scale = no::texture_size(fps_texture).to<float>();
	no::bind_texture(fps_texture);
	no::set_shader_model(transform);
	rectangle.bind();
	rectangle.draw();
	transform.position.y = 24.0f;
	transform.scale = no::texture_size(debug_texture).to<float>();
	no::bind_texture(debug_texture);
	no::set_shader_model(transform);
	rectangle.draw();

	for (auto& hit_splat : hit_splats) {
		hit_splat.draw(color, rectangle);
	}
	color.set(no::vector4f{ 1.0f });

	// background
	no::bind_texture(ui_texture);
	transform.position = 8.0f;
	transform.scale = hud_left_uv.zw;
	no::draw_shape(hud_left, transform);
	transform.position.x += transform.scale.x;
	transform.scale = hud_tile_1_uv.zw;
	for (int i = 0; i <= player->stat(stat_type::health).real() / 2; i++) {
		no::draw_shape(hud_tile_1, transform);
		transform.position.x += transform.scale.x;
	}
	transform.scale = hud_right_uv.zw;
	no::draw_shape(hud_right, transform);

	// health
	transform.scale = hud_health_size;
	transform.position = 8.0f;
	transform.position.x += 36.0f;
	transform.position.y += 32.0f;
	for (int i = 1; i <= player->stat(stat_type::health).real(); i++) {
		if (player->stat(stat_type::health).effective() >= i) {
			no::draw_shape(health_foreground, transform);
		} else {
			no::draw_shape(health_background, transform);
		}
		transform.position.x += transform.scale.x + 1.0f;
	}
}

void hud_view::set_fps(long long fps) {
	if (this->fps == fps) {
		return;
	}
	no::load_texture(fps_texture, font.render("FPS: " + std::to_string(fps)));
}

void hud_view::set_debug(const std::string& debug) {
	no::load_texture(debug_texture, font.render(debug));
}

user_interface_view::user_interface_view(game_state& game) : 
	game(game), inventory{ game }, equipment{ game }, font(no::asset_path("fonts/leo.ttf"), 9), minimap{ game.world } {
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
	delete context;
}

bool user_interface_view::is_mouse_over() const {
	no::transform2 transform;
	transform.scale = background_uv.zw;
	transform.position.x = game.ui_camera.width() - transform.scale.x - 2.0f;
	return transform.collides_with(game.ui_camera.mouse_position(game.mouse()));
}

bool user_interface_view::is_mouse_over_context() const {
	return context && context->menu_transform().collides_with(game.ui_camera.mouse_position(game.mouse()));
}

bool user_interface_view::is_mouse_over_inventory() const {
	return inventory.body_transform().collides_with(game.ui_camera.mouse_position(game.mouse()));
}

bool user_interface_view::is_tab_hovered(int index) const {
	return tab_transform(index).collides_with(game.ui_camera.mouse_position(game.mouse()));
}

bool user_interface_view::is_mouse_over_any() const {
	return is_mouse_over() || is_mouse_over_context();
}

bool user_interface_view::is_context_open() const {
	return context != nullptr;
}

void user_interface_view::listen(int object_id_) {
	object_id = object_id_;
	auto player = game.world.objects.character(object_id);
	equipment_event = player->events.equip.listen([this](const item_instance& event) {
		
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
	hud.set_fps(((const game_state&)game).frame_counter().current_fps());
	hud.set_debug(STRING("Tile: " << game.world.my_player().object.tile()));
	hud.update(game.ui_camera);
	minimap.transform.position = { 104.0f, 8.0f };
	minimap.transform.position.x += game.ui_camera.width() - background_uv.z - 2.0f;
	minimap.transform.scale = 64.0f;
	minimap.transform.rotation = game.world_camera().transform.rotation.y;
	minimap.update(game.world.my_player().object.tile());
	update_trading_ui();
}

void user_interface_view::draw() {
	no::bind_shader(shader);
	no::set_shader_view_projection(game.ui_camera);
	color.set(no::vector4f{ 1.0f });
	no::bind_texture(ui_texture);
	no::transform2 transform;
	transform.scale = background_uv.zw;
	transform.position.x = game.ui_camera.width() - transform.scale.x - 2.0f;
	no::set_shader_model(transform);
	background.bind();
	background.draw();
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
		break;
	}
	draw_tabs();
	hud.draw(color, ui_texture, game.world.objects.character(object_id));
	draw_trading_ui();
	color.set(no::vector4f{ 1.0f });
	no::bind_texture(ui_texture);
	if (context) {
		context->draw(ui_texture);
	}
	minimap.draw();
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
	transform.position.x += game.ui_camera.width() - background_uv.z - 2.0f + (tab_size.x + 4.0f) * (float)index;
	transform.scale = tab_size;
	return transform;
}

void user_interface_view::create_context() {
	delete context;
	context = new context_menu(game.ui_camera, game.ui_camera.mouse_position(game.mouse()), font, game.mouse());
	switch (tabs.active) {
	case 0: inventory.add_context_options(*context); break;
	case 1: equipment.add_context_options(*context); break;
	}
	add_trading_context_options(*context);
	if (!is_mouse_over_inventory()) {
		no::vector2i tile = game.hovered_tile();
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
					context->add_option("Attack " + name, [this, target_id] {
						game.start_combat(target_id);
					});
				}
				if (definition.id == 1) {
					context->add_option("Trade with " + name, [this, target_id] {
						game.send_trade_request(target_id);
					});
				}
			}
			if (definition.script_id.dialogue != -1) {
				std::string option_name = "Use ";
				if (definition.type == game_object_type::character) {
					option_name = "Talk to ";
				}
				context->add_option(option_name + name, [&] {
					game.start_dialogue(definition.script_id.dialogue);
				});
			}
			if (!definition.description.empty()) {
				context->add_option("Examine " + name, [&] {
					game.chat.add("", definition.description);
				});
			}
		});
	}
	if (context->count() == 0) {
		delete context;
		context = nullptr;
	} else {
		context->add_option("Cancel", [] {});
	}
}
