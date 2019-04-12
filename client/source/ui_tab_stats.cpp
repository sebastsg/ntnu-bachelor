#include "ui_tab_stats.hpp"
#include "ui_main.hpp"
#include "game.hpp"
#include "game_assets.hpp"

const no::vector2f stat_icon_uvs[] = {
	{ 8.0f, 192.0f }, // health
	{ 40.0f, 192.0f }, // stamina
	{ 104.0f, 224.0f }, // sword
	{ 40.0f, 224.0f }, // shield
	{ 72.0f, 224.0f }, // axe
	{ 136.0f, 224.0f }, // spear
	{ 8.0f, 224.0f } // fishing
};

const no::vector4f exp_back_uv{ 8.0f, 167.0f, 109.0f, 9.0f };
const no::vector4f exp_front_uv{ 8.0f, 177.0f, 109.0f, 9.0f };

const int stat_count = (int)stat_type::total;

struct stats_view {

	game_state& game;
	no::rectangle icons[stat_count];
	no::rectangle blank;
	std::vector<no::text_view> levels;
	no::rectangle exp_back;
	no::rectangle exp[stat_count];
	no::text_view exp_text;

	stats_view(game_state& game) : game{ game }, exp_text{ game, game.ui_camera } {}

};

static stats_view* stats = nullptr;

static no::transform2 body_transform() {
	no::transform2 transform;
	transform.scale = inventory_uv.zw;
	transform.position.x = stats->game.ui_camera.width() - background_uv.z - 2.0f + inventory_offset.x;
	transform.position.y = inventory_offset.y;
	return transform;
}

static no::transform2 stat_transform(int index) {
	no::transform2 transform;
	transform.position = body_transform().position;
	transform.position.x += 4.0f;
	transform.position.y += (float)index * 28.0f + 12.0f;
	transform.scale = 16.0f;
	return transform;
}

void show_stats_tab(game_state& game) {
	hide_stats_tab();
	stats = new stats_view{ game };
	for (int i = 0; i < stat_count; i++) {
		set_ui_uv(stats->icons[i], stat_icon_uvs[i], 32.0f);
		stats->levels.emplace_back(game, game.ui_camera);
	}
	set_ui_uv(stats->exp_back, exp_back_uv);
	set_ui_uv(stats->blank, { 0.0f, 0.0f, 4.0f, 4.0f });
}

void hide_stats_tab() {
	delete stats;
	stats = nullptr;
}

void update_stats_tab() {

}

void draw_stats_tab() {
	if (!stats) {
		return;
	}
	bool show_exp_text = false;
	auto& player = stats->game.world.my_player().character;
	for (int i = 0; i < stat_count; i++) {
		auto& stat = player.stat((stat_type)i);
		no::transform2 transform = stat_transform(i);
		no::bind_texture(sprites().ui);
		no::draw_shape(stats->icons[i], transform);
		transform.position.x += transform.scale.x + 2.0f;
		transform.position.y += 6.0f;
		transform.scale = exp_back_uv.zw;
		no::draw_shape(stats->exp_back, transform);
		no::vector2f mouse = stats->game.ui_camera.mouse_position(stats->game.mouse());
		bool mouse_over = transform.collides_with(mouse);
		float exp_width = stat.progress();
		set_ui_uv(stats->exp[i], exp_front_uv.xy, { exp_front_uv.z * exp_width, exp_front_uv.w });
		transform.scale.x = exp_front_uv.z * exp_width;
		no::draw_shape(stats->exp[i], transform);
		auto& level = stats->levels[i];
		transform = stat_transform(i);
		level.render(fonts().leo_9, STRING(stat.effective() << "/" << stat.real()));
		level.transform.position = transform.position + no::vector2f{ transform.scale.x + 4.0f, -4.0f };
		level.draw(shapes().rectangle);
		if (mouse_over) {
			stats->exp_text.render(fonts().leo_9, STRING(stat.experience_left() << " exp remaining to next level"));
			stats->exp_text.transform.position = mouse;
			stats->exp_text.transform.position.x -= stats->exp_text.transform.scale.x / 2.0f;
			stats->exp_text.transform.position.y += stats->exp_text.transform.scale.y + 2.0f;
			if (stats->exp_text.transform.position.x + stats->exp_text.transform.scale.x > stats->game.ui_camera.width()) {
				stats->exp_text.transform.position.x = stats->game.ui_camera.width() - stats->exp_text.transform.scale.x - 1.0f;
			}
			show_exp_text = true;
		}
	}
	if (show_exp_text) {
		no::transform2 transform = stats->exp_text.transform;
		transform.position -= no::vector2f{ 2.0f };
		transform.scale += no::vector2f{ 4.0f };
		auto color = no::get_shader_variable("uni_Color");
		color.set(no::vector4f{ 0.0f, 0.0f, 0.0f, 0.7f });
		no::bind_texture(sprites().ui);
		no::draw_shape(stats->blank, transform);
		color.set(no::vector4f{ 1.0f });
		stats->exp_text.draw(shapes().rectangle);
	}
}

void add_stats_context_menu_options() {

}

bool is_mouse_over_stats() {
	if (!stats) {
		return false;
	}
	return body_transform().collides_with(stats->game.ui_camera.mouse_position(stats->game.mouse()));
}
