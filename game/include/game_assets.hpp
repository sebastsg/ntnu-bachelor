#pragma once

#include "../config.hpp"

#if ENABLE_RENDERING

#include "assets.hpp"
#include "font.hpp"
#include "draw.hpp"

struct sprite_assets {
	int blank = -1;
	int blank_red = -1;
	int ui = -1;
};

struct font_assets {
	no::font leo_9;
	no::font leo_10;
	no::font leo_14;
	no::font leo_16;
};

struct shader_assets {
	struct {
		int id = -1;
		no::shader_variable color;
	} sprite;
};

struct shape_assets {
	no::rectangle rectangle;
};

void create_game_assets();
void destroy_game_assets();

sprite_assets& sprites();
font_assets& fonts();
shader_assets& shaders();
shape_assets& shapes();

#endif
