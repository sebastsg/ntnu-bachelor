#pragma once

#include "libpq-fe.h"

#include "math.hpp"
#include "gamevar.hpp"
#include "character.hpp"

#include <string>
#include <functional>

class query_result_row {
public:

	query_result_row(PGresult* result, int row);

	int integer(const std::string& column) const;
	std::string text(const std::string& column) const;
	char* raw(const std::string& column) const;
	int length(const std::string& column) const;
	bool exists(const std::string& column) const;

private:

	int column_index(const std::string& column) const;

	PGresult* result = nullptr;
	int row = 0;

};

class query_result {
public:

	query_result(PGresult* result);
	query_result(const query_result&) = delete;
	query_result(query_result&&) = delete;

	~query_result();

	query_result& operator=(const query_result&) = delete;
	query_result& operator=(query_result&&) = delete;

	query_result_row row(int index) const;
	void for_each(const std::function<void(const query_result_row&)>& function) const;
	int count() const;
	bool is_bad() const;
	std::string status_message() const;

private:

	PGresult* result = nullptr;

};

class database_connection {
public:

	database_connection();
	database_connection(const database_connection&) = delete;
	database_connection(database_connection&&) = delete;

	~database_connection();

	database_connection& operator=(const database_connection&) = delete;
	database_connection& operator=(database_connection&&) = delete;

	query_result execute(const std::string& query) const;
	query_result execute(const std::string& query, const std::initializer_list<std::string>& params) const;
	query_result call(const std::string& procedure, const std::initializer_list<std::string>& params) const;
	
	// todo: async alternatives

	bool is_bad() const;
	std::string status_message() const;

private:

	PGconn* connection = nullptr;

};

class game_persister {
public:

	game_persister(const database_connection& database);

	no::vector2i load_player_tile(int player_id);
	void save_player_tile(int player_id, no::vector2i tile);

	game_variable_map load_player_variables(int player_id);
	void save_player_variables(int player_id, const game_variable_map& variables);

	void load_player_items(int player_id, int container, item_container& items);
	void save_player_items(int player_id, int container, item_container& items);

private:

	const database_connection& database;

};
