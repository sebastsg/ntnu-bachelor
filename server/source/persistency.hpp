#pragma once

#include "libpq-fe.h"

#include <string>
#include <functional>

class query_result_row {
public:

	query_result_row(PGresult* result, int row);

	int integer(const std::string& column) const;
	std::string text(const std::string& column) const;
	char* raw(const std::string& column) const;
	int column_index(const std::string& column) const;
	bool exists(const std::string& column) const;

private:

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

	using callback_function = std::function<void(const query_result&)>;

	database_connection();
	database_connection(const database_connection&) = delete;
	database_connection(database_connection&&) = delete;

	~database_connection();

	database_connection& operator=(const database_connection&) = delete;
	database_connection& operator=(database_connection&&) = delete;

	void execute(const std::string& query, const callback_function& callback);
	void execute(const std::string& query, const callback_function& callback, const std::initializer_list<std::string>& params);
	bool is_bad() const;
	std::string status_message() const;

private:

	PGconn* connection = nullptr;

};
