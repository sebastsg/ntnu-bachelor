#pragma once

#include "io.hpp"

#include <functional>

struct quest_task {
	std::string name;
	int id = -1;
	int goal = 1;
	bool optional = false;
};

struct quest_hint {
	int unlocked_by_task_id = -1;
	std::string message;
};

class quest_task_list {
public:

	quest_task& add(const std::string& name);
	quest_task* find(int id);
	quest_task* find_by_index(int index);
	int find_index_by_id(int id) const;
	void remove(int id);
	int count() const;
	int next_id() const;

	void write(no::io_stream& stream) const;
	void read(no::io_stream& stream);

private:

	std::vector<quest_task> tasks;
	int id_counter = 0;

};

class quest_definition {
public:

	std::string name;
	int id = -1;
	std::vector<quest_hint> hints;
	quest_task_list tasks;
	
	void write(no::io_stream& stream) const;
	void read(no::io_stream& stream);

};

class quest_definition_list {
public:

	quest_definition_list();

	quest_definition& add(const std::string& name);
	quest_definition* find(int id);
	quest_definition* find_by_index(int index);
	int find_index_by_id(int id) const;
	int count() const;
	int next_id() const;

	void write(no::io_stream& stream) const;
	void read(no::io_stream& stream);

	void save() const;
	void load();

private:

	std::vector<quest_definition> quests;
	int id_counter = 0;

};

quest_definition_list& quest_definitions();

struct quest_task_instance {
	int task_id = -1;
	int progress = 0;
};

class quest_instance {
public:

	quest_instance(int id);
	quest_instance(no::io_stream& stream);

	int id() const;
	quest_definition& definition() const;

	quest_task_instance* find_task(int task_id);
	const quest_task_instance* find_task(int task_id) const;
	void for_each_task(const std::function<void(const quest_task_instance&)>& function) const;

	bool is_task_done(int task_id) const;
	bool is_done(bool require_optionals) const;
	void add_task_progress(int task_id, int progress);

	void write(no::io_stream& stream) const;
	void read(no::io_stream& stream);

private:

	int quest_id = -1;
	std::vector<quest_task_instance> tasks;

};

class quest_instance_list {
public:

	quest_instance* find(int quest_id);
	void for_each(const std::function<void(const quest_instance&)>& function) const;

	void write(no::io_stream& stream) const;
	void read(no::io_stream& stream);

private:

	std::vector<quest_instance> quests;

};
