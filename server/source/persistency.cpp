#include "persistency.hpp"
#include "network.hpp"
#include "debug.hpp"
#include "../config.hpp"

query_result_row::query_result_row(PGresult* result, int row) : result(result), row(row) {

}

int query_result_row::integer(const std::string& column) const {
	return std::stoi(raw(column));
}

long long query_result_row::long_integer(const std::string& column) const {
	return std::stoll(raw(column));
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
	namespace db = config::database;
	connection = PQconnectdb(CSTRING(
		"host=" << db::host <<
		" port=" << db::port <<
		" dbname=" << db::name <<
		" user=" << db::user <<
		" password=" << db::password
	));
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

void game_persister::load_player_items(int player_id, int container, item_instance* items, int count) {
	auto result = database.execute("select * from item_ownership where player_id = $1 and container = $2", {
		std::to_string(player_id),
		std::to_string(container)
	});
	for (int i = 0; i < result.count(); i++) {
		auto row = result.row(i);
		int slot = row.integer("slot");
		if (slot < 0 || slot >= count) {
			WARNING(player_id << " has invalid slot " << slot << " in " << container);
			continue;
		}
		items[slot] = { row.integer("item"), row.integer("stack") };
	}
}

void game_persister::save_player_items(int player_id, int container, item_instance* items, int count) {
	for (int i = 0; i < count; i++) {
		database.call("set_item_ownership", {
			std::to_string(player_id),
			std::to_string(container),
			std::to_string(i),
			std::to_string(items[i].definition_id),
			std::to_string(items[i].stack)
		 });
	}
}

quest_instance_list game_persister::load_player_quests(int player_id) {
	quest_instance_list quests;
	auto result = database.execute("select * from quest_task where player_id = $1", { std::to_string(player_id) });
	for (int i = 0; i < result.count(); i++) {
		auto row = result.row(i);
		auto quest = quests.find(row.integer("quest"));
		quest->add_task_progress(row.integer("task"), row.integer("progress"));
	}
	return quests;
}

void game_persister::save_player_quests(int player_id, quest_instance_list& quests) {
	quests.for_each([&](const quest_instance& quest) {
		quest.for_each_task([&](const quest_task_instance& task) {
			database.call("set_quest_task", {
				std::to_string(player_id),
				std::to_string(quest.id()),
				std::to_string(task.task_id),
				std::to_string(task.progress)
			});
		});
	});
}

void game_persister::load_player_stats(int player_id, character_object& character) {
	auto result = database.execute("select stat_type, experience from stat where player_id = $1", { std::to_string(player_id) });
	for (int i = 0; i < result.count(); i++) {
		auto row = result.row(i);
		stat_type stat = (stat_type)row.integer("stat_type");
		character.stat(stat).set_experience(row.long_integer("experience"));
	}
}

void game_persister::save_player_stats(int player_id, character_object& character) {
	for (int i = 0; i < (int)stat_type::total; i++) {
		database.call("set_stat", {
			std::to_string(player_id),
			std::to_string(i),
			std::to_string(character.stat((stat_type)i).experience())
		});
	}
}
