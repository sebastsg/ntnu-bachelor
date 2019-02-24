#pragma once

#include "io.hpp"

#include <unordered_map>

enum class variable_comparison { 
	equal, 
	not_equal, 
	greater_than, 
	less_than, 
	equal_or_greater_than, 
	equal_or_less_than
};

enum class variable_type { string, integer, floating, boolean };
enum class variable_modification { set, negate, add, multiply, divide };

struct game_variable {

	variable_type type = variable_type::string;
	std::string name;
	std::string value;
	bool is_persistent = true;

	bool compare(const std::string& right, variable_comparison comp_operator) const;
	void modify(const std::string& value, variable_modification mod_operator);

};

class game_variable_map {
public:

	game_variable* global(const std::string& name);
	game_variable* local(int scope_id, const std::string& name);

	void create_global(game_variable var);
	void create_local(int scope_id, game_variable var);

	void delete_global(const std::string& name);
	void delete_local(int scope_id, const std::string& name);

	void write(no::io_stream& stream);
	void read(no::io_stream& stream);

private:

	std::vector<game_variable> globals;
	std::unordered_map<int, std::vector<game_variable>> locals;

};
