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

int query_result_row::column_index(const std::string& column) const {
	return PQfnumber(result, column.c_str());
}

bool query_result_row::exists(const std::string& column) const {
	return column_index(column) != -1;
}

query_result::query_result(PGresult* result) : result(result) {

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
	return PQresultStatus(result) != PGRES_TUPLES_OK;
}

std::string query_result::status_message() const {
	return PQresultErrorMessage(result);
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

void database_connection::execute(const std::string& query, const std::function<void(const query_result&)>& callback) {
	query_result result{ PQexec(connection, query.c_str()) };
	if (result.is_bad()) {
		WARNING("Query failed. Error: " << result.status_message());
	} else {
		callback(result);
	}
}

void database_connection::execute(const std::string& query, const callback_function& callback, const std::initializer_list<std::string>& params) {
	int count = (int)params.size();
	Oid* types = nullptr; // deduced by backend
	const char** values = new const char*[count];
	for (int i = 0; i < count; i++) {
		values[i] = (params.begin() + i)->c_str();
	}
	int* lengths = nullptr; // we're sending c strings, no need
	int* formats = nullptr; // sending text, no need
	int result_format = 0; // text result
	query_result result{ PQexecParams(connection, query.c_str(), count, types, values, lengths, formats, result_format) };
	if (result.is_bad()) {
		WARNING("Query failed. Error: " << result.status_message());
	} else {
		callback(result);
	}
	delete[] values;
}

bool database_connection::is_bad() const {
	return PQstatus(connection) != CONNECTION_OK;
}

std::string database_connection::status_message() const {
	return PQerrorMessage(connection);
}
