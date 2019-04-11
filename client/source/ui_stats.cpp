#include "ui_stats.hpp"
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

stats_view::stats_view(game_state& game) : game{ game }, exp_text{ game, game.ui_camera } {
	for (int i = 0; i < stat_count; i++) {
		set_ui_uv(icons[i], stat_icon_uvs[i], 32.0f);
		levels.emplace_back(game, game.ui_camera);
	}
	set_ui_uv(exp_back, exp_back_uv);
	set_ui_uv(blank, { 0.0f, 0.0f, 4.0f, 4.0f });
}

stats_view::~stats_view() {

}

no::transform2 stats_view::body_transform() const {
	no::transform2 transform;
	transform.scale = inventory_uv.zw;
	transform.position.x = game.ui_camera.width() - background_uv.z - 2.0f + inventory_offset.x;
	transform.position.y = inventory_offset.y;
	return transform;
}

no::transform2 stats_view::stat_transform(int index) const {
	no::transform2 transform;
	transform.position = body_transform().position;
	transform.position.x += 4.0f;
	transform.position.y += (float)index * 28.0f + 12.0f;
	transform.scale = 16.0f;
	return transform;
}

void stats_view::draw() {
	bool show_exp_text = false;
	auto& player = game.world.my_player().character;
	for (int i = 0; i < stat_count; i++) {
		auto& stat = player.stat((stat_type)i);
		no::transform2 transform = stat_transform(i);
		no::bind_texture(sprites().ui);
		no::draw_shape(icons[i], transform);
		transform.position.x += transform.scale.x + 2.0f;
		transform.position.y += 6.0f;
		transform.scale = exp_back_uv.zw;
		no::draw_shape(exp_back, transform);
		no::vector2f mouse = game.ui_camera.mouse_position(game.mouse());
		bool mouse_over = transform.collides_with(mouse);
		float exp_width = stat.progress();
		set_ui_uv(exp[i], exp_front_uv.xy, { exp_front_uv.z * exp_width, exp_front_uv.w });
		transform.scale.x = exp_front_uv.z * exp_width;
		no::draw_shape(exp[i], transform);
		auto& level = levels[i];
		transform = stat_transform(i);
		level.render(fonts().leo_9, STRING(stat.effective() << "/" << stat.real()));
		level.transform.position = transform.position + no::vector2f{ transform.scale.x + 4.0f, -4.0f };
		level.draw(shapes().rectangle);
		if (mouse_over) {
			exp_text.render(fonts().leo_9, STRING(stat.experience_left() << " exp remaining to next level"));
			exp_text.transform.position = mouse;
			exp_text.transform.position.x -= exp_text.transform.scale.x / 2.0f;
			exp_text.transform.position.y += exp_text.transform.scale.y + 2.0f;
			if (exp_text.transform.position.x + exp_text.transform.scale.x > game.ui_camera.width()) {
				exp_text.transform.position.x = game.ui_camera.width() - exp_text.transform.scale.x - 1.0f;
			}
			show_exp_text = true;
		}
	}
	if (show_exp_text) {
		no::transform2 transform = exp_text.transform;
		transform.position -= no::vector2f{ 2.0f };
		transform.scale += no::vector2f{ 4.0f };
		auto color = no::get_shader_variable("uni_Color");
		color.set(no::vector4f{ 0.0f, 0.0f, 0.0f, 0.7f });
		no::bind_texture(sprites().ui);
		no::draw_shape(blank, transform);
		color.set(no::vector4f{ 1.0f });
		exp_text.draw(shapes().rectangle);
	}
}
