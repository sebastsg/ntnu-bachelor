#include "ui_dialogue.hpp"
#include "game.hpp"
#include "game_assets.hpp"

dialogue_view::dialogue_view(game_state& game_, const no::ortho_camera& camera_, int id) : game(game_), camera(camera_), message_view(game, camera) {
	auto& player = game.world.my_player().character;
	dialogue.quests = &game.quests;
	dialogue.variables = &game.variables;
	dialogue.player_object_id = player.object_id;
	dialogue.inventory = &player.inventory;
	dialogue.equipment = &player.equipment;
	dialogue.world = &game.world;
	dialogue.load(id);
	if (dialogue.id == -1) {
		open = false;
		return;
	}
	open = true;
	dialogue.events.choice.listen([this](const script_tree::choice_event& event) {
		current_choice = 0;
		current_choices = event.choices;
		message_view.render(fonts().leo_10, ((choice_node*)dialogue.nodes[dialogue.current_node()])->text);
		choice_views.clear();
		for (auto& choice : event.choices) {
			choice_views.emplace_back(game, camera).render(fonts().leo_10, choice.text);
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
	transform.position = camera.transform.position;
	transform.scale = camera.transform.scale;
	transform.position += transform.scale / 2.0f;
	transform.scale /= 2.0f;
	transform.position -= transform.scale / 2.0f;
	message_view.transform.position = transform.position + 16.0f;
	for (int i = 0; i < (int)choice_views.size(); i++) {
		choice_views[i].transform.position = {
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
	message_view.draw(shapes().rectangle);
	for (int i = 0; i < (int)choice_views.size(); i++) {
		shaders().sprite.color.set(current_choice == i ? active : inactive);
		choice_views[i].draw(shapes().rectangle);
	}
}

bool dialogue_view::is_open() const {
	return open;
}
