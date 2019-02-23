#include "gamevar.hpp"
#include "debug.hpp"
#include "loop.hpp"

namespace global {
static game_variable_map variables;
}

game_variable_map& variables() {
	return global::variables;
}

static bool compare_game_var_greater_than(variable_type type, const std::string& a, const std::string& b) {
	switch (type) {
	case variable_type::string: return a.size() > b.size();
	case variable_type::integer: return std::stoi(a) > std::stoi(b);
	case variable_type::floating: return std::stof(a) > std::stof(b);
	case variable_type::boolean: return a == "1";
	default: return false;
	}
}

static void modify_game_var_add(game_variable* var, const std::string& value) {
	switch (var->type) {
	case variable_type::string: var->value += value; return;
	case variable_type::integer: var->value = std::to_string(std::stoi(var->value) + std::stoi(value)); return;
	case variable_type::floating: var->value = std::to_string(std::stof(var->value) + std::stof(value)); return;
	case variable_type::boolean: var->value = std::to_string(std::stoi(var->value) + std::stoi(value)); return;
	default: return;
	}
}

static void modify_game_var_multiply(game_variable* var, const std::string& value) {
	switch (var->type) {
	case variable_type::string: WARNING("Multiplying a string does not make any sense."); return;
	case variable_type::integer: var->value = std::to_string(std::stoi(var->value) * std::stoi(value)); return;
	case variable_type::floating: var->value = std::to_string(std::stof(var->value) * std::stof(value)); return;
	case variable_type::boolean: var->value = std::to_string(std::stoi(var->value) * std::stoi(value)); return;
	default: return;
	}
}

static void modify_game_var_divide(game_variable* var, const std::string& value) {
	int value_int = std::stoi(value);
	float value_float = std::stof(value);
	if (value_int == 0 || value_float == 0.0f) {
		WARNING("Cannot divide " << var->name << " by " << value << " (division by zero)<br>Setting to 0.");
		var->value = "0"; // most likely the desired output
		return;
	}
	switch (var->type) {
	case variable_type::string: WARNING("Dividing a string does not make any sense."); return;
	case variable_type::integer: var->value = std::to_string(std::stoi(var->value) / value_int); return;
	case variable_type::floating: var->value = std::to_string(std::stof(var->value) / value_float); return;
	case variable_type::boolean: var->value = std::to_string(std::stoi(var->value) / value_int); return;
	default: return;
	}
}

bool game_variable::compare(const std::string& b, variable_comparison comp_operator) const {
	switch (comp_operator) {
	case variable_comparison::equal: return value == b;
	case variable_comparison::not_equal: return value != b;
	case variable_comparison::greater_than: return compare_game_var_greater_than(type, value, b);
	case variable_comparison::less_than: return compare_game_var_greater_than(type, b, value);
	case variable_comparison::equal_or_greater_than: return compare_game_var_greater_than(type, value, b) || value == b;
	case variable_comparison::equal_or_less_than: return compare_game_var_greater_than(type, b, value) || value == b;
	default: return false;
	}
}

void game_variable::modify(const std::string& new_value, variable_modification mod_operator) {
	switch (mod_operator) {
	case variable_modification::set: value = new_value; return;
	case variable_modification::negate: value = std::to_string(!std::stoi(value)); return;
	case variable_modification::add: modify_game_var_add(this, new_value); return;
	case variable_modification::multiply: modify_game_var_multiply(this, new_value); return;
	case variable_modification::divide: modify_game_var_divide(this, new_value); return;
	default: return;
	}
}

static void write_game_variable(no::io_stream& stream, game_variable* var) {
	stream.write((int32_t)var->type);
	stream.write(var->name);
	stream.write(var->value);
	stream.write<uint8_t>(var->is_persistent ? 1 : 0);
}

static game_variable read_game_variable(no::io_stream& stream) {
	game_variable var;
	var.type = (variable_type)stream.read<int32_t>();
	var.name = stream.read<std::string>();
	var.value = stream.read<std::string>();
	var.is_persistent = (stream.read<uint8_t>() != 0);
	return var;
}

game_variable* game_variable_map::global(const std::string& name) {
	for (auto& i : globals) {
		if (i.name == name) {
			return &i;
		}
	}
	return nullptr;
}

game_variable* game_variable_map::local(int scope_id, const std::string& name) {
	auto scope = locals.find(scope_id);
	if (scope == locals.end()) {
		return nullptr;
	}
	for (auto& var : scope->second) {
		if (var.name == name) {
			return &var;
		}
	}
	return nullptr;
}

void game_variable_map::create_global(game_variable var) {
	if (global(var.name)) {
		WARNING("Variable " << var.name << " already exists.");
		return;
	}
	globals.push_back(var);
}

void game_variable_map::create_local(int scope_id, game_variable var) {
	if (local(scope_id, var.name)) {
		WARNING("Variable " << var.name << " already exists.");
		return;
	}
	locals[scope_id].push_back(var);
}

void game_variable_map::delete_global(const std::string& name) {
	for (int i = 0; i < (int)globals.size(); i++) {
		if (globals[i].name == name) {
			globals.erase(globals.begin() + i);
			break;
		}
	}
}

void game_variable_map::delete_local(int scope_id, const std::string& name) {
	std::vector<game_variable>* scope_locals = &locals[scope_id];
	for (int i = 0; i < (int)scope_locals->size(); i++) {
		if (scope_locals->at(i).name == name) {
			scope_locals->erase(scope_locals->begin() + i);
			break;
		}
	}
}

void game_variable_map::write(no::io_stream& stream) {
	stream.write((int32_t)globals.size());
	for (auto& i : globals) {
		write_game_variable(stream, &i);
	}
	stream.write((int32_t)locals.size());
	for (auto& i : locals) {
		stream.write<int32_t>(i.first);
		stream.write((int32_t)i.second.size());
		for (auto& j : i.second) {
			write_game_variable(stream, &j);
		}
	}
}

void game_variable_map::read(no::io_stream& stream) {
	int global_count = stream.read<int32_t>();
	for (int i = 0; i < global_count; i++) {
		globals.push_back(read_game_variable(stream));
	}
	int scope_count = stream.read<int32_t>();
	for (int i = 0; i < scope_count; i++) {
		int scope_id = stream.read<int32_t>();
		int var_count = stream.read<int32_t>();
		for (int j = 0; j < var_count; j++) {
			locals[scope_id].push_back(read_game_variable(stream));
		}
	}
}
