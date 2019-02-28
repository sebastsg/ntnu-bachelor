#include "persistency.hpp"
#include "network.hpp"
#include "debug.hpp"

query_result_row::query_result_row(PGresult* result, int row) : result(result), row(row) {

}

int query_result_row::integer(const std::string& column) const {
	return std::stoi(raw(column));
}

std::string query_result_row::text(const std::string& column) const {
	return PQgetvalue(result, row, column_index(column));
}

char* query_result_row::raw(const std::string& column) const {
	return PQgetvalue(result, row, column_index(column));
}

int query_result_row::length(const std::string& column) const {
	return PQgetlength(result, row, column_index(column));
}

int query_result_row::column_index(const std::string& column) const {
	return PQfnumber(result, column.c_str());
}

bool query_result_row::exists(const std::string& column) const {
	return column_index(column) != -1;
}

query_result::query_result(PGresult* result) : result(result) {
	if (is_bad()) {
		WARNING("Query failed. Error: " << status_message());
	}
}

query_result::~query_result() {
	PQclear(result);
}

query_result_row query_result::row(int index) const {
	return { result, index };
}

void query_result::for_each(const std::function<void(const query_result_row&)>& function) const {
	for (int i = 0; i < count(); i++) {
		function(row(i));
	}
}

int query_result::count() const {
	return PQntuples(result);
}

bool query_result::is_bad() const {
	switch (PQresultStatus(result)) {
	case PGRES_BAD_RESPONSE:
	case PGRES_FATAL_ERROR:
	case PGRES_NONFATAL_ERROR:
		return true;
	default:
		return false;
	}
}

std::string query_result::status_message() const {
	return PQresultVerboseErrorMessage(result, PQERRORS_VERBOSE, PQSHOW_CONTEXT_ALWAYS);
}

database_connection::database_connection() {
	connection = PQconnectdb("host=localhost port=5432 dbname=einheri user=einheri password=einheri");
	if (is_bad()) {
		CRITICAL("Failed to connect: " << status_message());
	}
}

database_connection::~database_connection() {
	PQfinish(connection);
}

query_result database_connection::execute(const std::string& query) const {
	return { PQexec(connection, query.c_str()) };
}

query_result database_connection::execute(const std::string& query, const std::initializer_list<std::string>& params) const {
	ASSERT(16 > params.size());
	int count = (int)params.size();
	Oid* types = nullptr; // deduced by backend
	const char* values[16];
	for (int i = 0; i < count; i++) {
		values[i] = (params.begin() + i)->c_str();
	}
	int* lengths = nullptr; // we're sending c strings, no need
	int* formats = nullptr; // sending text, no need
	int result_format = 0; // text result
	return { PQexecParams(connection, query.c_str(), count, types, values, lengths, formats, result_format) };
}

query_result database_connection::call(const std::string& procedure, const std::initializer_list<std::string>& params) const {
	std::string query = "call " + procedure + " ( ";
	for (int i = 1; i <= (int)params.size(); i++) {
		query += "$" + std::to_string(i) + ",";
	}
	query[query.size() - 1] = ')';
	return execute(query, params);
}

bool database_connection::is_bad() const {
	return PQstatus(connection) != CONNECTION_OK;
}

std::string database_connection::status_message() const {
	return PQerrorMessage(connection);
}

game_persister::game_persister(const database_connection& database) : database(database) {

}

no::vector2i game_persister::load_player_tile(int player_id) {
	auto result = database.execute("select tile_x, tile_z from player where id = $1", { std::to_string(player_id) });
	if (result.count() != 1) {
		return {};
	}
	auto row = result.row(0);
	return { row.integer("tile_x"), row.integer("tile_z") };
}

void game_persister::save_player_tile(int player_id, no::vector2i tile) {
	database.call("set_player_tile", { std::to_string(player_id), std::to_string(tile.x), std::to_string(tile.y) });
}

game_variable_map game_persister::load_player_variables(int player_id) {
	auto result = database.execute("select * from variable where player_id = $1", { std::to_string(player_id) });
	game_variable_map variables;
	for (int i = 0; i < result.count(); i++) {
		auto row = result.row(i);
		int scope = row.integer("scope");
		std::string name = row.text("var_name");
		std::string value = row.text("var_value");
		int type = row.integer("var_type");
		game_variable variable{ (variable_type)type, name, value, true };
		INFO("Loaded " << variable.name << ":" << variable.value);
		if (scope == -1) {
			variables.create_global(variable);
		} else {
			variables.create_local(scope, variable);
		}
	}
	return variables;
}

void game_persister::save_player_variables(int player_id, const game_variable_map& variables) {
	variables.for_each_global([this, player_id](const game_variable& variable) {
		database.call("set_variable", {
			std::to_string(player_id),
			"-1",
			variable.name,
			variable.value,
			std::to_string((int)variable.type)
		});
	});
	variables.for_each_local([this, player_id](int scope, const game_variable& variable) {
		database.call("set_variable", {
			std::to_string(player_id),
			std::to_string(scope),
			variable.name,
			variable.value,
			std::to_string((int)variable.type)
		 });
	});
}
