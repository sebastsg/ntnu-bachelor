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
	transform.scale.xy = no::texture_size(texture).to<float>();
}

void text_view::draw(const no::rectangle& rectangle) const {
	no::bind_texture(texture);
	no::draw_shape(rectangle, transform);
}

dialogue_view::dialogue_view(game_state& game, const no::ortho_camera& camera, int id) 
	: game(game), camera(camera), font(no::asset_path("fonts/leo.ttf"), 10) {
	auto player = game.world.my_player();
	if (!player) {
		WARNING("No player");
		open = false;
		return;
	}
	dialogue.variables = &game.variables;
	dialogue.player = player;
	dialogue.inventory = &player->inventory;
	dialogue.equipment = &player->equipment;
	dialogue.load(id);
	if (dialogue.id == -1) {
		open = false;
		return;
	}
	open = true;
	dialogue.events.choice.listen([this](const dialogue_tree::choice_event& event) {
		current_choice = 0;
		current_choices = event.choices;
		message_view.render(font, ((choice_node*)dialogue.nodes[dialogue.current_node()])->text);
		choice_views.clear();
		for (auto& choice : event.choices) {
			choice_views.emplace_back().render(font, choice.text);
		}
	});
	dialogue.process_entry_point();
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
			if (current_choice + 1 < (int)current_choices.size()) {
				current_choice++;
			}
			break;
		case no::key::space:
		case no::key::enter:
		{
			int choice = current_choices[current_choice].node_id;
			dialogue.select_choice(choice);
			events.choose.emit(choice);
			break;
		}
		default:
			break;
		}
	});
}

dialogue_view::~dialogue_view() {
	game.keyboard().press.ignore(key_listener);
}

void dialogue_view::update() {
	if (dialogue.current_node() == -1) {
		open = false;
	}
	if (!open) {
		return;
	}
	transform.position.xy = camera.transform.position.xy;
	transform.scale.xy = camera.transform.scale.xy;
	transform.position.xy += transform.scale.xy / 2.0f;
	transform.scale.xy /= 2.0f;
	transform.position.xy -= transform.scale.xy / 2.0f;
	message_view.transform.position.xy = transform.position.xy + 16.0f;
	for (int i = 0; i < (int)choice_views.size(); i++) {
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
	for (int i = 0; i < (int)choice_views.size(); i++) {
		color.set(current_choice == i ? active : inactive);
		choice_views[i].draw(rectangle);
	}
}

bool dialogue_view::is_open() const {
	return open;
}
