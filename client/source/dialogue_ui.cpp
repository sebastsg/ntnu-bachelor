#include "dialogue_ui.hpp"
#include "game.hpp"

text_view::text_view() {
	texture = no::create_texture();
}

text_view::text_view(text_view&& that) {
	std::swap(transform, that.transform);
	std::swap(rendered_text, that.rendered_text);
	std::swap(texture, that.texture);
}

text_view::~text_view() {
	no::delete_texture(texture);
}

text_view& text_view::operator=(text_view&& that) {
	std::swap(transform, that.transform);
	std::swap(rendered_text, that.rendered_text);
	std::swap(texture, that.texture);
	return *this;
}

std::string text_view::text() const {
	return rendered_text;
}

void text_view::render(const no::font& font, const std::string& text) {
	if (text == rendered_text) {
		return;
	}
	rendered_text = text;
	no::load_texture(texture, font.render(rendered_text));
}

void text_view::draw(const no::rectangle& rectangle) const {
	no::bind_texture(texture);
	no::draw_shape(rectangle, transform);
}

dialogue_view::dialogue_view(game_state& game, const no::ortho_camera& camera, int id) 
	: game(game), camera(camera), font(no::asset_path("fonts/leo.ttf"), 10) {
	dialogue.load(id);
	if (dialogue.id == -1) {
		open = false;
		return;
	}
	start_dialogue();
	key_listener = game.keyboard().press.listen([this](const no::keyboard::press_message& event) {
		// todo: 1, 2, 3, 4, 5... etc to select choice
		switch (event.key) {
		case no::key::w:
		case no::key::up:
			if (current_choice > 0) {
				current_choice--;
			}
			break;
		case no::key::s:
		case no::key::down:
			if (current_choice + 1 < (int)choices.size()) {
				current_choice++;
			}
			break;
		case no::key::space:
		case no::key::enter:
			select_choice(choices[current_choice].node_id);
			break;
		default:
			break;
		}
	});
}

dialogue_view::~dialogue_view() {
	
}

int dialogue_view::process_non_ui_node(int id, node_type type) {
	auto node = dialogue.nodes[id];
	int out = node->process();
	if (out == -1) {
		return -1;
	}
	return node->get_output(out);
}

int dialogue_view::process_nodes_get_choice(int id, node_type type) {
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
		type = dialogue.nodes[id]->type();
		if (type == node_type::choice) {
			return id;
		}
	}
	return -1;
}

void dialogue_view::prepare_message() {
	std::vector<int> choice_ids;
	for (auto& i : dialogue.nodes[current_node_id]->out) {
		node_type out_type = dialogue.nodes[i.node_id]->type();
		if (out_type == node_type::choice) {
			choice_ids.push_back(i.node_id);
		} else {
			int traversed = process_nodes_get_choice(i.node_id, out_type);
			if (traversed != -1) {
				choice_ids.push_back(traversed);
			}
		}
	}
	message_view.render(font, ((choice_node*)dialogue.nodes[current_node_id])->text);

	choices.clear();
	choice_views.clear();
	current_choice = 0;

	for (auto& i : choice_ids) {
		choices.push_back({ ((choice_node*)dialogue.nodes[i])->text, i });
	}
	if (choices.size() == 0) {
		choices.push_back({ "Oops, I encountered a bug. Gotta go!", -1 });
	}
	for (auto& i : choices) {
		choice_views.emplace_back().render(font, i.text);
	}
}

void dialogue_view::select_choice(int node_id) {
	if (node_id == -1) {
		open = false;
		return;
	}
	current_node_id = dialogue.nodes[node_id]->get_first_output();
	while (current_node_id != -1) {
		node_type type = dialogue.nodes[current_node_id]->type();
		if (type == node_type::message) {
			prepare_message();
			return;
		}
		if (type == node_type::choice) {
			INFO("A choice cannot be the entry point of a dialogue.");
			break;
		}
		current_node_id = process_non_ui_node(current_node_id, type);
	}
	open = false;
}

void dialogue_view::process_entry_point() {
	int i = 0;
	while (true) {
		node_type type = dialogue.nodes[current_node_id]->type();
		if (type == node_type::message) {
			prepare_message();
			return;
		}
		if (type == node_type::choice) {
			INFO("A choice cannot be the entry point of a dialogue.");
			open = false;
			return;
		}
		current_node_id = process_non_ui_node(current_node_id, type);
		if (current_node_id == -1) {
			open = false;
			return;
		}
		i++;
	}
}

void dialogue_view::start_dialogue() {
	current_node_id = dialogue.start_node_id;
	process_entry_point();
}

void dialogue_view::update() {
	if (!open) {
		return;
	}
	transform.position.xy = camera.position();
	transform.scale.xy = camera.size();
	transform.position.xy += transform.scale.xy / 2.0f;
	transform.scale.xy /= 2.0f;
	transform.position.xy -= transform.scale.xy / 2.0f;

	message_view.transform.position.xy = transform.position.xy + 16.0f;

	for (int i = 0; i < (int)choices.size(); i++) {
		choice_views[i].transform.position.xy = {
			transform.position.x + 32.0f,
			transform.position.y + 64.0f + (float)i * 20.0f
		};
	}
}

void dialogue_view::draw() const {
	if (!open) {
		return;
	}
	const no::vector4f active{ 1.0f, 0.9f, 0.5f, 1.0f };
	const no::vector4f inactive{ 0.8f };
	const auto color = no::get_shader_variable("uni_Color");
	message_view.draw(rectangle);
	for (int i = 0; i < (int)choices.size(); i++) {
		color.set(current_choice == i ? active : inactive);
		choice_views[i].draw(rectangle);
	}
}

bool dialogue_view::is_open() const {
	return open;
}
