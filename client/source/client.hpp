#pragma once

#include "loop.hpp"
#include "network.hpp"

class client_state : public no::window_state {
public:

	client_state();

protected:

	no::io_socket& server();

};
