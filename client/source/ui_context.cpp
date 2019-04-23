#include "ui_context.hpp"
#include "ui_tabs.hpp"
#include "game.hpp"
#include "game_assets.hpp"

static const int top_begin = 0;
static const int top_tile_1 = 1;
static const int top_tile_2 = 2;
static const int top_tile_3 = 3;
static const int top_end = 4;
static const int highlight_begin = 5;
static const int highlight_tile = 6;
static const int highlight_end = 7;
static const int item_tile = 8;
static const int left_1 = 9;
static const int left_2 = 10;
static const int right_1 = 11;
static const int right_2 = 12;
static const int bottom_begin = 13;
static const int bottom_tile_1 = 14;
static const int bottom_tile_2 = 15;
static const int bottom_tile_3 = 16;
static const int bottom_end = 17;
static const int total_tiles = 18;

struct context_menu_option {
	std::string text;
	std::function<void()> action;
	int texture = -1;
};

struct context_menu {

	no::vector2f position;
	float max_width = 0.0f;
	std::vector<context_menu_option> options;
	std::vector<no::rectangle> rectangles;
	game_state& game;

	context_menu(game_state& game) : game{ game } {}

	~context_menu() {
		for (auto& option : options) {
			no::delete_texture(option.texture);
		}
	}

};

static context_menu* context = nullptr;

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

static float calculate_max_offset_x() {
	float x = context_uv[top_begin].z;
	while (context->max_width > x) {
		x += context_uv[top_tile_1].z;
		if (context->max_width > x) {
			x += context_uv[top_tile_3].z;
		}
	}
	x += context_uv[top_tile_2].z;
	return x;
}

static no::transform2 menu_transform() {
	no::transform2 transform;
	transform.position = context->position;
	transform.scale = {
		calculate_max_offset_x(),
		context_uv[top_begin].w + context_uv[item_tile].w * (float)context_menu_option_count() + context_uv[bottom_begin].w
	};
	return transform;
}

static no::transform2 option_transform(int index) {
	no::transform2 transform;
	transform.position = context->position;
	transform.position.x += context_uv[left_1].z;
	transform.position.y += context_uv[top_begin].w - 8.0f + (float)index * context_uv[item_tile].w;
	transform.scale = { calculate_max_offset_x(), context_uv[item_tile].w };
	return transform;
}

static void draw_border(int uv, no::vector2f offset) {
	no::draw_shape(context->rectangles[uv], no::transform2{ { context->position + offset }, context_uv[uv].zw });
}

static void draw_top_border() {
	no::vector2f offset;
	draw_border(top_begin, offset);
	offset.x += context_uv[top_begin].z;
	while (context->max_width > offset.x) {
		draw_border(top_tile_1, offset);
		offset.x += context_uv[top_tile_1].z;
		if (context->max_width > offset.x) {
			draw_border(top_tile_3, offset);
			offset.x += context_uv[top_tile_3].z;
		}
	}
	draw_border(top_tile_2, offset);
	offset.x += context_uv[top_tile_2].z;
	draw_border(top_end, offset);
}

static void draw_background() {
	no::transform2 transform;
	transform.position = context->position + no::vector2f{ context_uv[left_1].z, context_uv[top_tile_1].w };
	float height = context_uv[top_begin].w + (float)(context_menu_option_count() - 1) * context_uv[item_tile].w;
	transform.scale = { calculate_max_offset_x(), height };
	no::draw_shape(context->rectangles[item_tile], transform);
}

static void draw_side_borders() {
	no::vector2f offset;
	offset.y = context_uv[top_begin].w;
	float top_before_end_x = calculate_max_offset_x();
	for (int i = 0; i < context_menu_option_count(); i++) {
		offset.x = 0.0f;
		draw_border(left_1 + i % 1, offset);
		draw_border(right_1 + i % 1, { top_before_end_x, offset.y });
		offset.y += context_uv[right_1].z;
	}
}

static void draw_bottom_border() {
	no::vector2f offset;
	offset.y = context_uv[top_begin].w + (float)(context_menu_option_count() - 1) * context_uv[item_tile].w;
	draw_border(bottom_begin, offset);
	offset.x = context_uv[bottom_begin].z;
	while (context->max_width > offset.x) {
		draw_border(bottom_tile_1, offset);
		offset.x += context_uv[bottom_tile_1].z;
		if (context->max_width > offset.x) {
			draw_border(bottom_tile_3, offset);
			offset.x += context_uv[bottom_tile_3].z;
		}
	}
	draw_border(bottom_tile_2, offset);
	offset.x += context_uv[bottom_tile_2].z;
	draw_border(bottom_end, offset);
}

static void draw_highlighted_option(int option) {
	no::vector2f offset;
	offset.x = context_uv[left_1].z;
	offset.y = context_uv[top_tile_1].w + (float)option * context_uv[item_tile].w - 2.0f;
	no::bind_texture(sprites().ui);
	draw_border(highlight_begin, offset);
	offset.x += context_uv[highlight_begin].z;
	float max_offset_x = calculate_max_offset_x();
	while (max_offset_x > offset.x + 8.0f) {
		draw_border(highlight_tile, offset);
		offset.x += context_uv[highlight_tile].z;
	}
	draw_border(highlight_end, offset);
	shaders().sprite.color.set({ 1.0f, 0.7f, 0.3f, 1.0f });
}

static void draw_option(int option_index) {
	no::vector2f offset;
	offset.x = context_uv[left_1].z;
	offset.y = context_uv[top_tile_1].w - 2.0f; // two padding pixels
	auto& option = context->options[option_index];
	if (is_mouse_over_context_menu_option(option_index)) {
		draw_highlighted_option(option_index);
	}
	no::transform2 transform;
	transform.position = context->position + context_uv[top_tile_1].zw;
	transform.position.x -= 6.0f;
	transform.position.y += (float)option_index * context_uv[item_tile].w + 2.0f;
	transform.scale = no::texture_size(option.texture).to<float>();
	no::bind_texture(option.texture);
	no::draw_shape(shapes().rectangle, transform);
	offset.y += context_uv[item_tile].w;
	shaders().sprite.color.set(no::vector4f{ 1.0f });
}

void open_context_menu(game_state& game) {
	close_context_menu();
	context = new context_menu{ game };
	context->position = game.ui_camera_1x.mouse_position(game.mouse());
	context->position.floor();
	for (int i = 0; i < total_tiles; i++) {
		set_ui_uv(context->rectangles.emplace_back(), context_uv[i]);
	}
}

void close_context_menu() {
	delete context;
	context = nullptr;
}

bool is_context_menu_open() {
	return context != nullptr;
}

void add_context_menu_option(const std::string& text, const std::function<void()>& action) {
	if (!context) {
		return;
	}
	auto& option = context->options.emplace_back();
	option.text = text;
	option.action = action;
	option.texture = no::create_texture(fonts().leo_9.render(text));
	context->max_width = std::max(context->max_width, (float)no::texture_size(option.texture).x + context_uv[top_begin].z);
	if (context->position.x + context->max_width + 40.0f > context->game.ui_camera_1x.width()) {
		context->position.x = context->game.ui_camera_1x.width() - context->max_width - 40.0f;
	}
}

void trigger_context_menu_option(int index) {
	if (context && context->options[index].action) {
		context->options[index].action();
	}
}

bool is_mouse_over_context_menu_option(int index) {
	return context ? option_transform(index).collides_with(context->game.ui_camera_1x.mouse_position(context->game.mouse())) : false;
}

bool is_mouse_over_context_menu() {
	return context ? menu_transform().collides_with(context->game.ui_camera_1x.mouse_position(context->game.mouse())) : false;
}

void draw_context_menu() {
	if (!context) {
		return;
	}
	draw_background();
	draw_top_border();
	for (int i = 0; i < context_menu_option_count(); i++) {
		draw_option(i);
	}
	no::bind_texture(sprites().ui);
	draw_side_borders();
	draw_bottom_border();
}

int context_menu_option_count() {
	return context ? (int)context->options.size() : 0;
}
