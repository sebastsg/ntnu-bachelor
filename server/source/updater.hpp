#pragma once

#include "network.hpp"

constexpr int newest_client_version = 19022000; // yymmddxx

class client_updater {
public:

	client_updater(no::io_socket& client);
	client_updater(const client_updater&) = delete;
	client_updater(client_updater&&);

	~client_updater() = default;

	client_updater& operator=(const client_updater&) = delete;
	client_updater& operator=(client_updater&&);

	void update();
	bool is_done() const;

private:

	no::io_socket& client;
	std::vector<std::string> paths;
	bool done = false;
	int total_files = 0;
	int current_file = 0;

};
