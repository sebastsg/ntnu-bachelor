#include "dialogue.hpp"
#include "debug.hpp"
#include "assets.hpp"
#include "loop.hpp"
#include "character.hpp"
#include "world.hpp"

#include "imgui/imgui.h"
#include "imgui/imgui_platform.h"

#include <ctime>

namespace global {
static dialogue_meta_list dialogue_meta;
}

static abstract_node* create_dialogue_node(node_type type) {
	switch (type) {
	case node_type::message: return new message_node();
	case node_type::choice: return new choice_node();
	case node_type::has_item_condition: return new has_item_condition_node();
	case node_type::stat_condition: return new stat_condition_node();
	case node_type::inventory_effect: return new inventory_effect_node();
	case node_type::stat_effect: return new stat_effect_node();
	case node_type::warp_effect: return new warp_effect_node();
	case node_type::var_condition: return new var_condition_node();
	case node_type::modify_var: return new modify_var_node();
	case node_type::create_var: return new create_var_node();
	case node_type::var_exists: return new var_exists_node();
	case node_type::delete_var: return new delete_var_node();
	case node_type::random: return new random_node();
	case node_type::random_condition: return new random_condition_node();
	case node_type::quest_task_condition: return new quest_task_condition_node();
	case node_type::quest_done_condition: return new quest_done_condition_node();
	case node_type::quest_update_task_effect: return new quest_update_task_effect_node();
	default: return nullptr;
	}
}

void dialogue_meta_item::write(no::io_stream& stream) const {
	stream.write(name);
	stream.write<int32_t>(id);
}

void dialogue_meta_item::read(no::io_stream& stream) {
	name = stream.read<std::string>();
	id = stream.read<int32_t>();
}

dialogue_meta_list::dialogue_meta_list() {
	no::post_configure_event().listen([this] {
		load();
	});
}

dialogue_meta_item& dialogue_meta_list::add(const std::string& name) {
	dialogue_meta_item item{ name, id_counter };
	id_counter++;
	return items.emplace_back(item);
}

dialogue_meta_item* dialogue_meta_list::find(int id) {
	for (auto& item : items) {
		if (item.id == id) {
			return &item;
		}
	}
	return nullptr;
}

dialogue_meta_item* dialogue_meta_list::find_by_index(int index) {
	return (index >= 0 || index < count()) ? &items[index] : nullptr;
}

int dialogue_meta_list::find_index_by_id(int id) const {
	for (int i = 0; i < count(); i++) {
		if (items[i].id == id) {
			return i;
		}
	}
	return -1;
}

int dialogue_meta_list::count() const {
	return (int)items.size();
}

int dialogue_meta_list::next_id() const {
	return id_counter;
}

void dialogue_meta_list::write(no::io_stream& stream) const {
	stream.write<int32_t>(id_counter);
	stream.write((int32_t)items.size());
	for (auto& item : items) {
		item.write(stream);
	}
}

void dialogue_meta_list::read(no::io_stream& stream) {
	id_counter = stream.read<int32_t>();
	int count = stream.read<int32_t>();
	for (int i = 0; i < count; i++) {
		items.emplace_back().read(stream);
	}
}

void dialogue_meta_list::save() const {
	no::io_stream stream;
	write(stream);
	no::file::write(no::asset_path("dialogues.meta"), stream);
}

void dialogue_meta_list::load() {
	no::io_stream stream;
	no::file::read(no::asset_path("dialogues.meta"), stream);
	if (stream.size_left_to_read() > 0) {
		read(stream);
	}
}

dialogue_meta_list& dialogue_meta() {
	return global::dialogue_meta;
}

void abstract_node::write(no::io_stream& stream) {
	stream.write<int32_t>(id);
	stream.write(transform);
	stream.write((int32_t)out.size());
	for (auto& j : out) {
		stream.write<int32_t>(j.out_id);
		stream.write<int32_t>(j.node_id);
	}
}

void abstract_node::read(no::io_stream& stream) {
	id = stream.read<int32_t>();
	transform = stream.read<no::transform3>();
	int out_count = stream.read<int32_t>();
	for (int j = 0; j < out_count; j++) {
		node_output output;
		output.out_id = stream.read<int32_t>();
		output.node_id = stream.read<int32_t>();
		out.push_back(output);
	}
}

void abstract_node::remove_output_node(int node_id) {
	for (int i = 0; i < (int)out.size(); i++) {
		if (out[i].node_id == node_id) {
			out.erase(out.begin() + i);
			i--;
		}
	}
}

void abstract_node::remove_output_type(int out_id) {
	for (int i = 0; i < (int)out.size(); i++) {
		if (out[i].out_id == out_id) {
			out.erase(out.begin() + i);
			i--;
		}
	}
}

int abstract_node::get_output(int out_id) {
	for (auto& i : out) {
		if (i.out_id == out_id) {
			return i.node_id;
		}
	}
	return -1;
}

int abstract_node::get_first_output() {
	if (out.empty()) {
		return -1;
	}
	return out[0].node_id;
}

void abstract_node::set_output_node(int out_id, int node_id) {
	if (node_id == -1) {
		return;
	}
	if (out_id == -1) {
		out_id = 0;
		while (get_output(out_id) != -1) {
			out_id++;
		}
	}
	for (auto& i : out) {
		if (i.out_id == out_id) {
			i.node_id = node_id;
			return;
		}
	}
	node_output output;
	output.node_id = node_id;
	output.out_id = out_id;
	out.push_back(output);
}

void dialogue_tree::write(no::io_stream& stream) const {
	stream.write<int32_t>(id);
	stream.write((int32_t)nodes.size());
	for (auto& i : nodes) {
		stream.write((int32_t)i.second->type());
		i.second->write(stream);
	}
	stream.write<int32_t>(id_counter);
	stream.write<int32_t>(start_node_id);
}

void dialogue_tree::read(no::io_stream& stream) {
	id = stream.read<int32_t>();
	int node_count = stream.read<int32_t>();
	for (int i = 0; i < node_count; i++) {
		node_type type = (node_type)stream.read<int32_t>();
		auto node = create_dialogue_node(type);
		node->tree = this;
		node->read(stream);
		nodes[node->id] = node;
	}
	id_counter = stream.read<int32_t>();
	start_node_id = stream.read<int32_t>();
}

void dialogue_tree::save() const {
	no::io_stream stream;
	write(stream);
	no::file::write(no::asset_path(STRING("dialogues/" << id << ".ed")), stream);
}

void dialogue_tree::load(int id) {
	no::io_stream stream;
	no::file::read(no::asset_path(STRING("dialogues/" << id << ".ed")), stream);
	if (stream.size_left_to_read() > 0) {
		read(stream);
	}
}

int dialogue_tree::current_node() const {
	return current_node_id;
}

void dialogue_tree::select_choice(int node_id) {
	if (nodes.find(node_id) == nodes.end()) {
		WARNING("Node not found: " << node_id << ". Dialogue: " << id);
		return;
	}
	current_node_id = nodes[node_id]->get_first_output();
	while (process_choice_selection());
}

void dialogue_tree::process_entry_point() {
	current_node_id = start_node_id;
	while (process_choice_selection());
}

bool dialogue_tree::process_choice_selection() {
	if (current_node_id == -1) {
		return false;
	}
	node_type type = nodes[current_node_id]->type();
	if (type == node_type::message) {
		prepare_message();
		return false;
	}
	if (type == node_type::choice) {
		INFO("A choice cannot be the entry point of a dialogue.");
		return false;
	}
	current_node_id = process_non_ui_node(current_node_id, type);
	return current_node_id != -1;
}

void dialogue_tree::prepare_message() {
	choice_event event;
	std::vector<int> choices = process_current_and_get_choices();
	for (int choice : choices) {
		event.choices.push_back({ ((choice_node*)nodes[choice])->text, choice });
	}
	if (event.choices.empty()) {
		event.choices.push_back({ "Oops, I encountered a bug. Gotta go!", -1 });
	}
	events.choice.emit(event);
}

std::vector<int> dialogue_tree::process_current_and_get_choices() {
	std::vector<int> choices;
	for (auto& output : nodes[current_node_id]->out) {
		node_type out_type = nodes[output.node_id]->type();
		if (out_type == node_type::choice) {
			choices.push_back(output.node_id);
		} else {
			int traversed = process_nodes_get_choice(output.node_id, out_type);
			if (traversed != -1) {
				choices.push_back(traversed);
			}
		}
	}
	return choices;
}

int dialogue_tree::process_nodes_get_choice(int id, node_type type) {
	while (true) {
		if (id == -1 || type == node_type::message) {
			return -1;
		}
		if (type == node_type::choice) {
			return id;
		}
		id = process_non_ui_node(id, type);
		if (id == -1) {
			return -1;
		}
		type = nodes[id]->type();
		if (type == node_type::choice) {
			return id;
		}
	}
	return -1;
}

int dialogue_tree::process_non_ui_node(int id, node_type type) {
	auto node = nodes[id];
	int out = node->process();
	if (out == -1) {
		return -1;
	}
	return node->get_output(out);
}

void message_node::write(no::io_stream& stream) {
	abstract_node::write(stream);
	stream.write(text);
}

void message_node::read(no::io_stream& stream) {
	abstract_node::read(stream);
	text = stream.read<std::string>();
}

void choice_node::write(no::io_stream& stream) {
	abstract_node::write(stream);
	stream.write(text);
}

void choice_node::read(no::io_stream& stream) {
	abstract_node::read(stream);
	text = stream.read<std::string>();
}

int has_item_condition_node::process() {
	long long has_stack = 0;
	tree->inventory->for_each([&](no::vector2i slot, const item_instance& inventory_item) {
		if (inventory_item.definition_id == item.definition_id) {
			has_stack += inventory_item.stack;
		}
	});
	if (check_equipment_too) {
		tree->equipment->for_each([&](no::vector2i slot, const item_instance& equipment_item) {
			if (equipment_item.definition_id == item.definition_id) {
				has_stack += equipment_item.stack;
			}
		});
	}
	return has_stack >= item.stack ? 1 : 0;
}

void has_item_condition_node::write(no::io_stream& stream) {
	abstract_node::write(stream);
	stream.write<int32_t>(item.definition_id);
	stream.write<int64_t>(item.stack);
	stream.write<uint8_t>(check_equipment_too);
}

void has_item_condition_node::read(no::io_stream& stream) {
	abstract_node::read(stream);
	item.definition_id = stream.read<int32_t>();
	item.stack = stream.read<int64_t>();
	check_equipment_too = (stream.read<uint8_t>() != 0);
}

int stat_condition_node::process() {
	// todo: check stat
	return 1;
}

void stat_condition_node::write(no::io_stream& stream) {
	abstract_node::write(stream);
	stream.write<int32_t>(stat);
	stream.write<int32_t>(min_value);
	stream.write<int32_t>(max_value);
}

void stat_condition_node::read(no::io_stream& stream) {
	abstract_node::read(stream);
	stat = stream.read<int32_t>();
	min_value = stream.read<int32_t>();
	max_value = stream.read<int32_t>();
}

int inventory_effect_node::process() {
	if (give) {
		tree->inventory->add_from(item);
	} else {
		tree->inventory->remove_to(item.stack, item);
	}
	return 0;
}

void inventory_effect_node::write(no::io_stream& stream) {
	abstract_node::write(stream);
	stream.write<int32_t>(item.definition_id);
	stream.write<int64_t>(item.stack);
	stream.write<uint8_t>(give ? 1 : 0);
}

void inventory_effect_node::read(no::io_stream& stream) {
	abstract_node::read(stream);
	item.definition_id = stream.read<int32_t>();
	item.stack = stream.read<int64_t>();
	give = (stream.read<uint8_t>() != 0);
}

int stat_effect_node::process() {
	// todo: modify stat
	return 0;
}

void stat_effect_node::write(no::io_stream& stream) {
	abstract_node::write(stream);
	stream.write<int32_t>(stat);
	stream.write<int32_t>(value);
	stream.write<uint8_t>(assign);
}

void stat_effect_node::read(no::io_stream& stream) {
	abstract_node::read(stream);
	stat = stream.read<int32_t>();
	value = stream.read<int32_t>();
	assign = (stream.read<uint8_t>() != 0);
}

int warp_effect_node::process() {
	auto& player = tree->world->objects.object(tree->player_object_id);
	player.transform.position.x = tile.x;
	player.transform.position.z = tile.y;
	return 0;
}

void warp_effect_node::write(no::io_stream& stream) {
	abstract_node::write(stream);
	stream.write<int32_t>(world_id);
	stream.write(tile);
}

void warp_effect_node::read(no::io_stream& stream) {
	abstract_node::read(stream);
	world_id = stream.read<int32_t>();
	tile = stream.read<no::vector2f>();
}

int var_condition_node::process() {
	auto variables = tree->variables;
	game_variable* var = nullptr;
	if (is_global) {
		var = variables->global(var_name);
	} else {
		var = variables->local(scope_id, var_name);
	}
	if (!var) {
		WARNING("Attempted to check " << var_name << " (global: " << is_global << ") but it does not exist");
		return false;
	}
	if (comp_value == "") {
		return var->name == "";
	}
	std::string value = comp_value;
	if (other_type == node_other_var_type::local) {
		auto comp_var = variables->local(scope_id, comp_value);
		if (comp_var) {
			value = comp_var->value;
		} else {
			WARNING("Cannot compare against " << var_name << " because local variable " << comp_value << " does not exist.");
			return false;
		}
	} else if (other_type == node_other_var_type::global) {
		auto comp_var = variables->global(comp_value);
		if (comp_var) {
			value = comp_var->value;
		} else {
			WARNING("Cannot compare against " << var_name << " because global variable " << comp_value << " does not exist.");
			return false;
		}
	}
	return var->compare(value, comp_operator) ? 1 : 0;
}

void var_condition_node::write(no::io_stream& stream) {
	abstract_node::write(stream);
	stream.write<uint8_t>(is_global);
	stream.write((int32_t)other_type);
	stream.write(var_name);
	stream.write(comp_value);
	stream.write((int32_t)comp_operator);
}

void var_condition_node::read(no::io_stream& stream) {
	abstract_node::read(stream);
	is_global = (stream.read<uint8_t>() != 0);
	other_type = (node_other_var_type)stream.read<int32_t>();
	var_name = stream.read<std::string>();
	comp_value = stream.read<std::string>();
	comp_operator = (variable_comparison)stream.read<int32_t>();
}

int modify_var_node::process() {
	if (mod_value == "") {
		return 0;
	}
	auto variables = tree->variables;
	game_variable* var = nullptr;
	if (is_global) {
		var = variables->global(var_name);
	} else {
		var = variables->local(scope_id, var_name);
	}
	if (!var) {
		WARNING("Attempted to modify " << var_name << " (global: " << is_global << ") but it does not exist");
		return 0;
	}
	std::string value = mod_value;
	if (other_type == node_other_var_type::local) {
		auto mod_var = variables->local(scope_id, mod_value);
		if (mod_var) {
			value = mod_var->value;
		} else {
			WARNING("Cannot modify " << var_name << " because the local variable " << mod_value << " does not exist.");
			return 0;
		}
	} else if (other_type == node_other_var_type::global) {
		auto mod_var = variables->global(mod_value);
		if (mod_var) {
			value = mod_var->value;
		} else {
			WARNING("Cannot modify " << var_name << " because the global variable " << mod_value << " does not exist.");
			return 0;
		}
	}
	var->modify(value, mod_operator);
	return 0;
}

void modify_var_node::write(no::io_stream& stream) {
	abstract_node::write(stream);
	stream.write<uint8_t>(is_global);
	stream.write((int32_t)other_type);
	stream.write(var_name);
	stream.write(mod_value);
	stream.write((int32_t)mod_operator);
}

void modify_var_node::read(no::io_stream& stream) {
	abstract_node::read(stream);
	is_global = (stream.read<uint8_t>() != 0);
	other_type = (node_other_var_type)stream.read<int32_t>();
	var_name = stream.read<std::string>();
	mod_value = stream.read<std::string>();
	mod_operator = (variable_modification)stream.read<int32_t>();
}

int create_var_node::process() {
	auto variables = tree->variables;
	if (is_global) {
		auto old_var = variables->global(var.name);
		if (old_var) {
			if (overwrite) {
				*old_var = var;
			}
			return 0;
		}
		variables->create_global(var);
	} else {
		auto old_var = variables->local(scope_id, var.name);
		if (old_var) {
			if (overwrite) {
				*old_var = var;
			}
			return 0;
		}
		variables->create_local(scope_id, var);
	}
	return 0;
}

void create_var_node::write(no::io_stream& stream) {
	abstract_node::write(stream);
	stream.write<uint8_t>(is_global);
	stream.write<uint8_t>(overwrite);
	stream.write((int32_t)var.type);
	stream.write(var.name);
	stream.write(var.value);
	stream.write<uint8_t>(var.is_persistent);
}

void create_var_node::read(no::io_stream& stream) {
	abstract_node::read(stream);
	is_global = (stream.read<uint8_t>() != 0);
	overwrite = (stream.read<uint8_t>() != 0);
	var.type = (variable_type)stream.read<int32_t>();
	var.name = stream.read<std::string>();
	var.value = stream.read<std::string>();
	var.is_persistent = (stream.read<uint8_t>() != 0);
}

int var_exists_node::process() {
	if (is_global) {
		return tree->variables->global(var_name) != nullptr ? 1 : 0;
	}
	return tree->variables->local(scope_id, var_name) != nullptr ? 1 : 0;
}

void var_exists_node::read(no::io_stream& stream) {
	abstract_node::read(stream);
	is_global = (stream.read<uint8_t>() != 0);
	var_name = stream.read<std::string>();
}

void var_exists_node::write(no::io_stream& stream) {
	abstract_node::write(stream);
	stream.write<uint8_t>(is_global);
	stream.write(var_name);
}

int delete_var_node::process() {
	if (is_global) {
		tree->variables->delete_global(var_name);
	} else {
		tree->variables->delete_local(scope_id, var_name);
	}
	return 0;
}

void delete_var_node::write(no::io_stream& stream) {
	abstract_node::write(stream);
	stream.write<uint8_t>(is_global);
	stream.write(var_name);
}

void delete_var_node::read(no::io_stream& stream) {
	abstract_node::read(stream);
	is_global = (stream.read<uint8_t>() != 0);
	var_name = stream.read<std::string>();
}

int random_node::process() {
	if (out.size() == 0) {
		WARNING("No nodes attached.");
		return -1;
	}
	// todo: randomness
	return 0;
}

void random_node::write(no::io_stream& stream) {
	abstract_node::write(stream);
}

void random_node::read(no::io_stream& stream) {
	abstract_node::read(stream);
}

int random_condition_node::process() {
	// todo: randomness
	return 0;
}

void random_condition_node::write(no::io_stream& stream) {
	abstract_node::write(stream);
	stream.write<int32_t>(percent);
}

void random_condition_node::read(no::io_stream& stream) {
	abstract_node::read(stream);
	percent = stream.read<int32_t>();
}

int quest_task_condition_node::process() {
	auto quest = tree->quests->find(quest_id);
	if (quest) {
		return quest->is_task_done(task_id);
	}
	return 0;
}

void quest_task_condition_node::write(no::io_stream& stream) {
	abstract_node::write(stream);
	stream.write<int32_t>(quest_id);
	stream.write<int32_t>(task_id);
	stream.write<int32_t>(task_progress);
}

void quest_task_condition_node::read(no::io_stream& stream) {
	abstract_node::read(stream);
	quest_id = stream.read<int32_t>();
	task_id = stream.read<int32_t>();
	task_progress = stream.read<int32_t>();
}

int quest_done_condition_node::process() {
	auto quest = tree->quests->find(quest_id);
	if (quest) {
		return quest->is_done(require_optionals);
	}
	return 0;
}

void quest_done_condition_node::write(no::io_stream& stream) {
	abstract_node::write(stream);
	stream.write<int32_t>(quest_id);
	stream.write<uint8_t>(require_optionals ? 1 : 0);
}

void quest_done_condition_node::read(no::io_stream& stream) {
	abstract_node::read(stream);
	quest_id = stream.read<int32_t>();
	require_optionals = stream.read<uint8_t>() != 0;
}

int quest_update_task_effect_node::process() {
	auto quest = tree->quests->find(quest_id);
	if (quest) {
		quest->add_task_progress(task_id, task_progress);
	}
	return 0;
}

void quest_update_task_effect_node::write(no::io_stream& stream) {
	abstract_node::write(stream);
	stream.write<int32_t>(quest_id);
	stream.write<int32_t>(task_id);
	stream.write<int32_t>(task_progress);
}

void quest_update_task_effect_node::read(no::io_stream& stream) {
	abstract_node::read(stream);
	quest_id = stream.read<int32_t>();
	task_id = stream.read<int32_t>();
	task_progress = stream.read<int32_t>();
}
