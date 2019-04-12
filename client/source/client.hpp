#pragma once

#include "loop.hpp"

class client_state : public no::window_state {
public:

	client_state();

	std::string player_name() const;

protected:

	static struct player_details_ {
		std::string name;
	} player_details;

	int server();

};
