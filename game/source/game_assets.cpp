#include "game_assets.hpp"

#if ENABLE_RENDERING

#include "draw.hpp"

struct game_assets {
	
	sprite_assets sprites;
	font_assets fonts;
	shader_assets shaders;
	shape_assets* shapes = nullptr;

	void create() {
		sprites.blank = no::create_texture({ 2, 2, no::pixel_format::rgba, 0xFFFFFFFF });
		sprites.blank_red = no::create_texture({ 2, 2, no::pixel_format::rgba, 0xFF0000FF });
		sprites.ui = no::create_texture({ no::asset_path("sprites/ui.png") });

		fonts.leo_9 = { no::asset_path("fonts/leo.ttf"), 9 };
		fonts.leo_10 = { no::asset_path("fonts/leo.ttf"), 10 };
		fonts.leo_14 = { no::asset_path("fonts/leo.ttf"), 14 };
		fonts.leo_16 = { no::asset_path("fonts/leo.ttf"), 16 };

		shaders.sprite.id = no::create_shader(no::asset_path("shaders/sprite"));
		shaders.sprite.color = no::get_shader_variable("uni_Color");

		shapes = new shape_assets;
	}

	void destroy() {
		no::delete_texture(sprites.blank);
		no::delete_texture(sprites.blank_red);
		no::delete_texture(sprites.ui);
		no::delete_shader(shaders.sprite.id);
		delete shapes;
	}

};

static game_assets assets;

void create_game_assets() {
	assets.create();
}

void destroy_game_assets() {
	assets.destroy();
}

sprite_assets& sprites() {
	return assets.sprites;
}

font_assets& fonts() {
	return assets.fonts;
}

shader_assets& shaders() {
	return assets.shaders;
}

shape_assets& shapes() {
	return *assets.shapes;
}

#endif
