#include "quest.hpp"
#include "assets.hpp"
#include "loop.hpp"

namespace global {
static quest_definition_list quest_definitions;
}

quest_task& quest_task_list::add(const std::string& name) {
	quest_task item{ name, id_counter };
	id_counter++;
	return tasks.emplace_back(item);
}

quest_task* quest_task_list::find(int id) {
	for (auto& task : tasks) {
		if (task.id == id) {
			return &task;
		}
	}
	return nullptr;
}

quest_task* quest_task_list::find_by_index(int index) {
	return (index >= 0 || index < count()) ? &tasks[index] : nullptr;
}

int quest_task_list::find_index_by_id(int id) const {
	for (int i = 0; i < count(); i++) {
		if (tasks[i].id == id) {
			return i;
		}
	}
	return -1;
}

void quest_task_list::remove(int id) {
	int index = find_index_by_id(id);
	if (index != -1) {
		tasks.erase(tasks.begin() + index);
	}
}

int quest_task_list::count() const {
	return (int)tasks.size();
}

int quest_task_list::next_id() const {
	return id_counter;
}

void quest_task_list::write(no::io_stream& stream) const {
	stream.write<int32_t>(id_counter);
	stream.write<int32_t>(count());
	for (auto& task : tasks) {
		stream.write(task.name);
		stream.write<int32_t>(task.id);
		stream.write<int32_t>(task.goal);
		stream.write<uint8_t>(task.optional ? 1 : 0);
	}
}

void quest_task_list::read(no::io_stream& stream) {
	int id_counter = stream.read<int32_t>();
	int count = stream.read<int32_t>();
	for (int i = 0; i < count; i++) {
		quest_task task;
		task.name = stream.read<std::string>();
		task.id = stream.read<int32_t>();
		task.goal = stream.read<int32_t>();
		task.optional = stream.read<uint8_t>() != 0;
		tasks.push_back(task);
	}
}

void quest_definition::write(no::io_stream& stream) const {
	stream.write(name);
	stream.write<int32_t>(id);
	tasks.write(stream);
	stream.write((int32_t)hints.size());
	for (auto& hint : hints) {
		stream.write<int32_t>(hint.unlocked_by_task_id);
		stream.write(hint.message);
	}
}

void quest_definition::read(no::io_stream& stream) {
	name = stream.read<std::string>();
	id = stream.read<int32_t>();
	tasks.read(stream);
	int count = stream.read<int32_t>();
	for (int i = 0; i < count; i++) {
		quest_hint hint;
		hint.unlocked_by_task_id = stream.read<int32_t>();
		hint.message = stream.read<std::string>();
		hints.push_back(hint);
	}
}

quest_definition_list& quest_definitions() {
	return global::quest_definitions;
}

quest_definition_list::quest_definition_list() {
	no::post_configure_event().listen([this] {
		load();
	});
}

quest_definition& quest_definition_list::add(const std::string& name) {
	quest_definition item{ name, id_counter };
	id_counter++;
	return quests.emplace_back(item);
}

quest_definition* quest_definition_list::find(int id) {
	for (auto& quest : quests) {
		if (quest.id == id) {
			return &quest;
		}
	}
	return nullptr;
}

quest_definition* quest_definition_list::find_by_index(int index) {
	return (index >= 0 || index < count()) ? &quests[index] : nullptr;
}

int quest_definition_list::find_index_by_id(int id) const {
	for (int i = 0; i < count(); i++) {
		if (quests[i].id == id) {
			return i;
		}
	}
	return -1;
}

int quest_definition_list::count() const {
	return (int)quests.size();
}

int quest_definition_list::next_id() const {
	return id_counter;
}

void quest_definition_list::write(no::io_stream& stream) const {
	stream.write<int32_t>(id_counter);
	stream.write((int32_t)quests.size());
	for (auto& quest : quests) {
		quest.write(stream);
	}
}

void quest_definition_list::read(no::io_stream& stream) {
	id_counter = stream.read<int32_t>();
	int count = stream.read<int32_t>();
	for (int i = 0; i < count; i++) {
		quests.emplace_back().read(stream);
	}
}

void quest_definition_list::save() const {
	no::io_stream stream;
	write(stream);
	no::file::write(no::asset_path("quests.data"), stream);
}

void quest_definition_list::load() {
	no::io_stream stream;
	no::file::read(no::asset_path("quests.data"), stream);
	if (stream.size_left_to_read() > 0) {
		read(stream);
	}
}

quest_instance::quest_instance(int id) : quest_id(id) {
	ASSERT(id != -1);
}

quest_instance::quest_instance(no::io_stream& stream) {
	read(stream);
}

int quest_instance::id() const {
	return quest_id;
}

quest_definition& quest_instance::definition() const {
	return *quest_definitions().find(quest_id);
}

quest_task_instance* quest_instance::find_task(int task_id) {
	for (auto& task : tasks) {
		if (task.task_id == task_id) {
			return &task;
		}
	}
	return nullptr;
}

const quest_task_instance* quest_instance::find_task(int task_id) const {
	for (auto& task : tasks) {
		if (task.task_id == task_id) {
			return &task;
		}
	}
	return nullptr;
}

void quest_instance::for_each_task(const std::function<void(const quest_task_instance&)>& function) const {
	for (auto& task : tasks) {
		function(task);
	}
}

bool quest_instance::is_task_done(int task_id) const {
	for (auto& task : tasks) {
		if (task.task_id == task_id) {
			return task.progress >= definition().tasks.find(task_id)->goal;
		}
	}
	return false;
}

bool quest_instance::is_done(bool require_optionals) const {
	for (auto& task : tasks) {
		auto task_definition = definition().tasks.find(task.task_id);
		if (task_definition && task_definition->goal > task.progress) {
			if (require_optionals || !task_definition->optional) {
				return false;
			}
		}
	}
	return true;
}

void quest_instance::add_task_progress(int task_id, int progress) {
	auto task = find_task(task_id);
	if (!task) {
		auto definition = quest_definitions().find(quest_id)->tasks.find(task_id);
		if (!definition) {
			return;
		}
		task = &tasks.emplace_back();
		task->task_id = definition->id;
	}
	task->progress += progress;
}

void quest_instance::write(no::io_stream& stream) const {
	stream.write<int32_t>(quest_id);
	stream.write((int32_t)tasks.size());
	for (auto& task : tasks) {
		stream.write<int32_t>(task.task_id);
		stream.write<int32_t>(task.progress);
	}
}

void quest_instance::read(no::io_stream& stream) {
	quest_id = stream.read<int32_t>();
	int count = stream.read<int32_t>();
	for (int i = 0; i < count; i++) {
		auto& task = tasks.emplace_back();
		task.task_id = stream.read<int32_t>();
		task.progress = stream.read<int32_t>();
	}
}

quest_instance* quest_instance_list::find(int quest_id) {
	for (auto& quest : quests) {
		if (quest.id() == quest_id) {
			return &quest;
		}
	}
	if (quest_definitions().find(quest_id)) {
		return &quests.emplace_back(quest_id);
	}
	return nullptr;
}

void quest_instance_list::for_each(const std::function<void(const quest_instance&)>& function) const {
	for (auto& quest : quests) {
		function(quest);
	}
}

void quest_instance_list::write(no::io_stream& stream) const {
	stream.write((int32_t)quests.size());
	for (auto& quest : quests) {
		quest.write(stream);
	}
}

void quest_instance_list::read(no::io_stream& stream) {
	int count = stream.read<int32_t>();
	for (int i = 0; i < count; i++) {
		quests.emplace_back(stream);
	}
}
