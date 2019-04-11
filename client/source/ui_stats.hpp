#pragma once

#include "draw.hpp"
#include "character.hpp"
#include "ui.hpp"

class game_state;

class stats_view {
public:

	stats_view(game_state& game);
	stats_view(const stats_view&) = delete;
	stats_view(stats_view&&) = delete;

	~stats_view();

	stats_view& operator=(const stats_view&) = delete;
	stats_view& operator=(stats_view&&) = delete;
	
	no::transform2 body_transform() const;
	no::transform2 stat_transform(int index) const;

	void draw();

private:

	static const int stat_count = (int)stat_type::total;

	game_state& game;

	no::rectangle icons[stat_count];
	no::rectangle blank;
	std::vector<no::text_view> levels;

	no::rectangle exp_back;
	no::rectangle exp[stat_count];
	no::text_view exp_text;

};
