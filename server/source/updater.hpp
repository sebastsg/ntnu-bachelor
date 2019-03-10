#pragma once

#include <vector>

constexpr int newest_client_version = 19030600; // yymmddxx

class client_updater {
public:

	client_updater(int client);
	client_updater(const client_updater&) = delete;
	client_updater(client_updater&&);

	~client_updater() = default;

	client_updater& operator=(const client_updater&) = delete;
	client_updater& operator=(client_updater&&);

	void update();
	bool is_done() const;

private:

	int client = -1;
	std::vector<std::string> paths;
	bool done = false;
	int total_files = 0;
	int current_file = 0;

};
