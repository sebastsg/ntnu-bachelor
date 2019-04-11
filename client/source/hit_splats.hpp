#pragma once

class game_state;

void start_hit_splats(game_state& game);
void stop_hit_splats();
void add_hit_splat(int target_id, int value);
void update_hit_splats();
void draw_hit_splats();
