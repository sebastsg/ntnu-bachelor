#pragma once

#include "loop.hpp"
#include "network.hpp"
#include "packets.hpp"

constexpr int client_version = 19022000; // yymmddxx

class file_transfer {
public:

	file_transfer(const std::string& path);

	void update(const file_transfer_packet& packet);

	std::string relative_path() const;
	std::string full_path() const;
	bool is_completed() const;

private:

	std::string path;
	no::io_stream stream;
	size_t total_received = 0;
	bool completed = false;

};

class updater_state : public no::window_state {
public:

	updater_state();
	~updater_state() override;

	void update() override;
	void draw() override;

private:

	file_transfer& transfer_for_path(const std::string& path);
	void erase_transfer(const std::string& path);

	no::io_socket server;
	std::vector<file_transfer> transfers;
	int completed_transfers = 0;
	no::timer previous_packet;

};
