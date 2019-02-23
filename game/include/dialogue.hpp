#pragma once

#include "transform.hpp"
#include "io.hpp"

#include "item.hpp"
#include "gamevar.hpp"

#include <unordered_set>

enum class node_type {
	message,
	choice,
	has_item_condition,
	stat_condition,
	inventory_effect,
	stat_effect,
	warp_effect,
	var_condition,
	modify_var,
	create_var,
	var_exists,
	delete_var,
	random,
	random_condition
};

enum class node_output_type { unknown, variable, single, boolean };
enum class node_other_var_type { value, local, global };

class dialogue_meta_item {
public:

	std::string name;
	int id = 0;

	void write(no::io_stream& stream) const;
	void read(no::io_stream& stream);

};

class dialogue_meta_list {
public:

	dialogue_meta_list();

	dialogue_meta_item& add(const std::string& name);
	dialogue_meta_item* find(int id);
	dialogue_meta_item* find_by_index(int index);
	int find_index_by_id(int id) const;
	int count() const;
	int next_id() const;

	void write(no::io_stream& stream) const;
	void read(no::io_stream& stream);

	void save() const;
	void load();

private:

	std::vector<dialogue_meta_item> items;
	int id_counter = 0;

};

dialogue_meta_list& dialogue_meta();

struct node_output {
	int node_id = -1;
	int out_id = 0;
};

class abstract_node {
public:

	int id = -1;
	int scope_id = -1;
	std::vector<node_output> out;

	// while transform is only used in editor, it's practical to keep here
	no::transform transform;

	virtual node_type type() const = 0;
	virtual node_output_type output_type() const = 0;

	virtual int process() {
		return -1;
	}

	virtual void write(no::io_stream& stream);
	virtual void read(no::io_stream& stream);

	void remove_output_node(int node_id);
	void remove_output_type(int out_id);
	int get_output(int out_id);
	int get_first_output();
	void set_output_node(int out_id, int node_id);

};

class condition_node : public abstract_node {
public:

	node_output_type output_type() const override {
		return node_output_type::boolean;
	}

};

class effect_node : public abstract_node {
public:

	node_output_type output_type() const override {
		return node_output_type::single;
	}

};

class message_node : public abstract_node {
public:

	std::string text = "Example text";

	node_type type() const override {
		return node_type::message;
	}

	node_output_type output_type() const override {
		return node_output_type::variable;
	}

	void write(no::io_stream& stream) override;
	void read(no::io_stream& stream) override;

};

class choice_node : public abstract_node {
public:

	std::string text = "Example text";

	node_type type() const override {
		return node_type::choice;
	}

	node_output_type output_type() const override {
		return node_output_type::single;
	}

	void write(no::io_stream& stream) override;
	void read(no::io_stream& stream) override;

};

class has_item_condition_node : public condition_node {
public:

	bool check_equipment_too = false;
	item_instance item;

	node_type type() const override {
		return node_type::has_item_condition;
	}

	int process() override;
	void write(no::io_stream& stream) override;
	void read(no::io_stream& stream) override;

};

class stat_condition_node : public condition_node {
public:

	int stat = 0;
	int min_value = 0;
	int max_value = 0;

	node_type type() const override {
		return node_type::stat_condition;
	}

	int process() override;
	void write(no::io_stream& stream) override;
	void read(no::io_stream& stream) override;

};

class inventory_effect_node : public effect_node {
public:

	bool give = true; // if false, the item will be taken. todo: enum?
	item_instance item;
	
	node_type type() const override {
		return node_type::inventory_effect;
	}

	int process() override;
	void write(no::io_stream& stream) override;
	void read(no::io_stream& stream) override;

};

class stat_effect_node : public effect_node {
public:

	int stat = 0;
	int value = 0;
	bool assign = false; // adds value to stat if false, otherwise it is set to value. todo: enum?
	
	node_type type() const override {
		return node_type::stat_effect;
	}

	int process() override;
	void write(no::io_stream& stream) override;
	void read(no::io_stream& stream) override;

};

class warp_effect_node : public effect_node {
public:

	int world_id = 0;
	no::vector2i tile;

	node_type type() const override {
		return node_type::warp_effect;
	}

	int process() override;
	void write(no::io_stream& stream) override;
	void read(no::io_stream& stream) override;

};

class var_condition_node : public condition_node {
public:

	bool is_global = false;
	node_other_var_type other_type = node_other_var_type::value;
	std::string var_name;
	std::string comp_value;
	variable_comparison comp_operator = variable_comparison::equal;

	node_type type() const override {
		return node_type::var_condition;
	}

	int process() override;
	void write(no::io_stream& stream) override;
	void read(no::io_stream& stream) override;

};

class modify_var_node : public effect_node {
public:

	bool is_global = false;
	node_other_var_type other_type = node_other_var_type::value;
	std::string var_name;
	std::string mod_value;
	variable_modification mod_operator = variable_modification::set;

	node_type type() const override {
		return node_type::modify_var;
	}

	int process() override;
	void write(no::io_stream& stream) override;
	void read(no::io_stream& stream) override;

};

class create_var_node : public effect_node {
public:

	game_variable var;
	bool is_global = false;
	bool overwrite = false;

	node_type type() const override {
		return node_type::create_var;
	}

	int process() override;
	void write(no::io_stream& stream) override;
	void read(no::io_stream& stream) override;

};

class var_exists_node : public condition_node {
public:

	bool is_global = false;
	std::string var_name;

	node_type type() const override {
		return node_type::var_exists;
	}

	int process() override;
	void write(no::io_stream& stream) override;
	void read(no::io_stream& stream) override;

};

class delete_var_node : public effect_node {
public:

	bool is_global = false;
	std::string var_name;

	node_type type() const override {
		return node_type::delete_var;
	}

	int process() override;
	void write(no::io_stream& stream) override;
	void read(no::io_stream& stream) override;

};

class random_node : public abstract_node {
public:

	node_type type() const override {
		return node_type::random;
	}

	node_output_type output_type() const override {
		return node_output_type::variable;
	}

	int process() override;
	void write(no::io_stream& stream) override;
	void read(no::io_stream& stream) override;

};

class random_condition_node : public condition_node {
public:

	int percent = 50;

	node_type type() const override {
		return node_type::random_condition;
	}

	int process() override;
	void write(no::io_stream& stream) override;
	void read(no::io_stream& stream) override;

};

struct dialogue_tree {

	int id = -1;
	int id_counter = 0;
	int start_node_id = 0; // todo: when deleting node, make sure start node is valid
	std::unordered_map<int, abstract_node*> nodes;

	void write(no::io_stream& stream) const;
	void read(no::io_stream& stream);

	void save() const;
	void load(int id);

};

struct dialogue_choice_button {

	std::string text;
	int node_id = -1;

};
