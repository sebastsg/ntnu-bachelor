#include "ui_hud.hpp"
#include "ui_tabs.hpp"
#include "game.hpp"
#include "game_assets.hpp"
#include "minimap.hpp"

const no::vector4f hud_left_uv = { 8.0f, 528.0f, 40.0f, 68.0f };
const no::vector4f hud_tile_1_uv = { 56.0f, 528.0f, 16.0f, 68.0f };
const no::vector4f hud_tile_2_uv = { 80.0f, 528.0f, 16.0f, 68.0f };
const no::vector4f hud_right_uv = { 104.0f, 528.0f, 16.0f, 68.0f };
const no::vector2f hud_health_background = { 104.0f, 152.0f };
const no::vector2f hud_health_foreground = { 112.0f, 152.0f };
const no::vector2f hud_health_size = { 8.0f, 12.0f };

struct hud_view {

	game_state& game;
	world_minimap minimap;

	long long fps = 0;

	no::rectangle left;
	no::rectangle tile_1;
	no::rectangle tile_2;
	no::rectangle right;
	no::rectangle health_background;
	no::rectangle health_foreground;
	no::rectangle background;

	int fps_texture = -1;
	int debug_texture = -1;

	hud_view(game_state& game) : game{ game }, minimap{ game.world } {}

	~hud_view() {
		no::delete_texture(fps_texture);
		no::delete_texture(debug_texture);
	}

};

static hud_view* hud = nullptr;

void show_hud(game_state& game) {
	hide_hud();
	hud = new hud_view{ game };
	hud->fps_texture = no::create_texture();
	hud->debug_texture = no::create_texture();
	set_ui_uv(hud->left, hud_left_uv);
	set_ui_uv(hud->tile_1, hud_tile_1_uv);
	set_ui_uv(hud->tile_2, hud_tile_2_uv);
	set_ui_uv(hud->right, hud_right_uv);
	set_ui_uv(hud->health_background, hud_health_background, hud_health_size);
	set_ui_uv(hud->health_foreground, hud_health_foreground, hud_health_size);
	set_ui_uv(hud->background, background_uv);
}

void hide_hud() {
	delete hud;
	hud = nullptr;
}

void update_hud() {
	hud->minimap.transform.position = { 109.0f, 12.0f };
	hud->minimap.transform.position.x += hud->game.ui_camera.width() - background_uv.z - 2.0f;
	hud->minimap.transform.scale = 64.0f;
	hud->minimap.transform.rotation = hud->game.world_camera().transform.rotation.y;
	hud->minimap.update(hud->game.world.my_player().object.tile());
}

void draw_hud() {
	hud->minimap.draw();

	auto& player = hud->game.world.my_player().character;
	no::transform2 transform;
	transform.position = { 300.0f, 4.0f };
	transform.scale = no::texture_size(hud->fps_texture).to<float>();
	no::bind_texture(hud->fps_texture);
	no::draw_shape(shapes().rectangle, transform);
	transform.position.y = 24.0f;
	transform.scale = no::texture_size(hud->debug_texture).to<float>();
	no::bind_texture(hud->debug_texture);
	no::set_shader_model(transform);
	shapes().rectangle.draw();

	// background
	no::bind_texture(sprites().ui);
	transform.position = 8.0f;
	transform.scale = hud_left_uv.zw;
	no::draw_shape(hud->left, transform);
	transform.position.x += transform.scale.x;
	transform.scale = hud_tile_1_uv.zw;
	for (int i = 0; i <= player.stat(stat_type::health).real() / 2; i++) {
		no::draw_shape(hud->tile_1, transform);
		transform.position.x += transform.scale.x;
	}
	transform.scale = hud_right_uv.zw;
	no::draw_shape(hud->right, transform);

	// health
	transform.scale = hud_health_size;
	transform.position = 8.0f;
	transform.position.x += 36.0f;
	transform.position.y += 32.0f;
	for (int i = 1; i <= player.stat(stat_type::health).real(); i++) {
		if (player.stat(stat_type::health).effective() >= i) {
			no::draw_shape(hud->health_foreground, transform);
		} else {
			no::draw_shape(hud->health_background, transform);
		}
		transform.position.x += transform.scale.x + 1.0f;
	}

	// right background
	transform.scale = background_uv.zw;
	transform.position.x = hud->game.ui_camera.width() - transform.scale.x - 2.0f;
	transform.position.y = 0.0f;
	no::draw_shape(hud->background, transform);
}

void set_hud_fps(long long fps) {
	if (hud->fps == fps) {
		return;
	}
	no::load_texture(hud->fps_texture, fonts().leo_9.render("FPS: " + std::to_string(fps)));
}

void set_hud_debug(const std::string & debug) {
	no::load_texture(hud->debug_texture, fonts().leo_9.render(debug));
}
