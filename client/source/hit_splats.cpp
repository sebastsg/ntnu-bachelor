#include "hit_splats.hpp"
#include "draw.hpp"
#include "game.hpp"
#include "game_assets.hpp"

struct hit_splat {

	no::transform2 transform;
	int target_id = -1;
	int texture = -1;
	float wait = 0.0f;
	float fade_in = 0.0f;
	float stay = 0.0f;
	float fade_out = 0.0f;
	float alpha = 0.0f;

	bool hit_splat::is_visible() const {
		return fade_out < 1.0f;
	}

};

static std::vector<hit_splat> splats;
static game_state* game = nullptr;

void start_hit_splats(game_state& game_) {
	game = &game_;
}

void stop_hit_splats() {
	for (auto& splat : splats) {
		no::delete_texture(splat.texture);
	}
	splats.clear();
}

void add_hit_splat(int target_id, int value) {
	auto& splat = splats.emplace_back();
	splat.target_id = target_id;
	splat.texture = no::create_texture(fonts().leo_14.render(std::to_string(value)));
}

void update_hit_splats() {
	for (int i = 0; i < (int)splats.size(); i++) {
		auto& splat = splats[i];
		if (splat.wait < 1.0f) {
			splat.wait += 0.04f;
			splat.alpha = 0.0f;
		} else if (splat.fade_in < 1.0f) {
			splat.fade_in += 0.02f;
			splat.alpha = splat.fade_in;
		} else if (splat.stay < 1.0f) {
			splat.stay += 0.04f;
			splat.alpha = 1.0f;
		} else if (splat.fade_out < 1.0f) {
			splat.fade_out += 0.03f;
			splat.alpha = 1.0f - splat.fade_out;
		}
		if (splat.target_id != -1) {
			auto& target = game->world.objects.object(splat.target_id);
			no::vector3f position = target.transform.position;
			position.y += 2.0f; // todo: height of model
			splat.transform.position = game->world_camera().world_to_screen(position) / game->ui_camera_2x.zoom;
			splat.transform.position.y -= (splat.fade_in * 0.5f + splat.fade_out) * 32.0f;
		}
		splat.transform.scale = no::texture_size(splat.texture).to<float>();
		if (!splat.is_visible()) {
			no::delete_texture(splat.texture);
			splats.erase(splats.begin() + i);
			i--;
		}
	}
}

void draw_hit_splats() {
	for (auto& splat : splats) {
		auto background_transform = splat.transform;
		background_transform.position -= 2.0f;
		background_transform.scale.x += 2.0f;
		no::bind_texture(sprites().blank_red);
		shaders().sprite.color.set({ 1.0f, 1.0f, 1.0f, splat.alpha * 0.75f });
		no::draw_shape(shapes().rectangle, background_transform);
		auto shadow_transform = splat.transform;
		shadow_transform.position += 1.0f;
		no::bind_texture(splat.texture);
		shaders().sprite.color.set({ 0.0f, 0.0f, 0.0f, splat.alpha });
		no::draw_shape(shapes().rectangle, shadow_transform);
		shaders().sprite.color.set({ 1.0f, 1.0f, 1.0f, splat.alpha });
		no::draw_shape(shapes().rectangle, splat.transform);
	}
}
