#include "ui_hud.hpp"
#include "ui_tabs.hpp"
#include "game.hpp"
#include "game_assets.hpp"
#include "minimap.hpp"

const no::vector4f hud_left_uv = { 755.0f, 290.0f, 29.0f, 60.0f };
const no::vector4f hud_tile_uv = { 792.0f, 290.0f, 1.0f, 60.0f };
const no::vector4f hud_right_uv = { 800.0f, 290.0f, 29.0f, 60.0f };
const no::vector2f hud_health_background = { 690.0f, 288.0f };
const no::vector2f hud_health_foreground = { 680.0f, 288.0f };
const no::vector2f hud_health_size = { 10.0f, 10.0f };
const no::vector4f minimap_uv = { 792.0f, 8.0f, 96.0f, 96.0f };

struct hud_view {

	game_state& game;
	world_minimap minimap;

	long long fps = 0;

	no::rectangle left;
	no::rectangle tile;
	no::rectangle right;
	no::rectangle health_background;
	no::rectangle health_foreground;
	no::rectangle minimap_background;

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
	set_ui_uv(hud->tile, hud_tile_uv);
	set_ui_uv(hud->right, hud_right_uv);
	set_ui_uv(hud->health_background, hud_health_background, hud_health_size);
	set_ui_uv(hud->health_foreground, hud_health_foreground, hud_health_size);
	set_ui_uv(hud->minimap_background, minimap_uv);
}

void hide_hud() {
	delete hud;
	hud = nullptr;
}

void update_hud() {
	hud->minimap.transform.position.x = hud->game.ui_camera_2x.width() - minimap_uv.z + 15.0f;
	hud->minimap.transform.position.y = 18.0f;
	hud->minimap.transform.scale = 64.0f;
	hud->minimap.transform.rotation = hud->game.world_camera().transform.rotation.y;
	hud->minimap.update(hud->game.world.my_player().object.tile());
}

void draw_hud() {
	hud->minimap.draw();
	no::bind_texture(sprites().ui);
	no::transform2 transform;
	transform.scale = minimap_uv.zw;
	transform.position.x = hud->game.ui_camera_2x.width() - transform.scale.x - 2.0f;
	transform.position.y = 2.0f;
	no::draw_shape(hud->minimap_background, transform);

	auto& player = hud->game.world.my_player().character;
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
	transform.scale.x = (float)player.stat(stat_type::health).real() * 10.0f;
	transform.scale.y = hud_tile_uv.w;
	no::draw_shape(hud->tile, transform);
	transform.position.x += transform.scale.x;
	transform.scale = hud_right_uv.zw;
	no::draw_shape(hud->right, transform);

	// health
	transform.scale = hud_health_size;
	transform.position.x = 36.0f;
	transform.position.y = 20.0f;
	for (int i = 1; i <= player.stat(stat_type::health).real(); i++) {
		if (player.stat(stat_type::health).effective() >= i) {
			no::draw_shape(hud->health_foreground, transform);
		} else {
			no::draw_shape(hud->health_background, transform);
		}
		transform.position.x += transform.scale.x + 1.0f;
		if (i % 18 == 0) {
			transform.position.x = 36.0f;
			transform.position.y += 12.0f;
		}
	}
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
