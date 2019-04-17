#include "ui_dialogue.hpp"
#include "game.hpp"
#include "game_assets.hpp"
#include "script.hpp"
#include "ui.hpp"

struct dialogue_view {

	struct {
		no::message_event<int> choose;
	} events;

	game_state& game;
	int key_listener = -1;

	script_tree dialogue;
	int current_choice = 0;
	std::vector<node_choice_info> current_choices;
	no::transform2 transform;

	no::text_view message_view;
	std::vector<no::text_view> choice_views;

	dialogue_view(game_state& game) : game{ game }, message_view{ game, game.ui_camera } {}

	~dialogue_view() {
		game.keyboard().press.ignore(key_listener);
	}

};

static dialogue_view* dialogue = nullptr;

void open_dialogue(game_state& game, int id) {
	close_dialogue();
	dialogue = new dialogue_view{ game };
	auto& player = game.world.my_player().character;
	auto& tree = dialogue->dialogue;
	tree.quests = &game.quests;
	tree.variables = &game.variables;
	tree.player_object_id = player.object_id;
	tree.inventory = &player.inventory;
	tree.equipment = &player.equipment;
	tree.world = &game.world;
	tree.load(id);
	if (tree.id == -1) {
		close_dialogue();
		return;
	}
	tree.events.choice.listen([&tree](const script_tree::choice_event& event) {
		dialogue->current_choice = 0;
		dialogue->current_choices = event.choices;
		dialogue->message_view.render(fonts().leo_10, ((choice_node*)tree.nodes[tree.current_node()])->text);
		dialogue->choice_views.clear();
		for (auto& choice : event.choices) {
			dialogue->choice_views.emplace_back(dialogue->game, dialogue->game.ui_camera).render(fonts().leo_10, choice.text);
		}
	});
	tree.process_entry_point();
	dialogue->key_listener = game.keyboard().press.listen([&tree](const no::keyboard::press_message& event) {
		// todo: 1, 2, 3, 4, 5... etc to select choice
		switch (event.key) {
		case no::key::w:
		case no::key::up:
			if (dialogue->current_choice > 0) {
				dialogue->current_choice--;
			}
			break;
		case no::key::s:
		case no::key::down:
			if (dialogue->current_choice + 1 < (int)dialogue->current_choices.size()) {
				dialogue->current_choice++;
			}
			break;
		case no::key::space:
		case no::key::enter:
		{
			int choice = dialogue->current_choices[dialogue->current_choice].node_id;
			tree.select_choice(choice);
			dialogue->events.choose.emit(choice);
			break;
		}
		default:
			break;
		}
	});
}

void close_dialogue() {
	if (!dialogue) {
		return;
	}
	delete dialogue;
	dialogue = nullptr;
}

bool is_dialogue_open() {
	return dialogue != nullptr;
}

void update_dialogue() {
	if (!dialogue) {
		return;
	}
	if (dialogue->dialogue.current_node() == -1) {
		close_dialogue();
		return;
	}
	dialogue->transform.position = dialogue->game.ui_camera.transform.position;
	dialogue->transform.scale = dialogue->game.ui_camera.transform.scale;
	dialogue->transform.position += dialogue->transform.scale / 2.0f;
	dialogue->transform.scale /= 2.0f;
	dialogue->transform.position -= dialogue->transform.scale / 2.0f;
	dialogue->message_view.transform.position = dialogue->transform.position + 16.0f;
	for (int i = 0; i < (int)dialogue->choice_views.size(); i++) {
		dialogue->choice_views[i].transform.position = {
			dialogue->transform.position.x + 32.0f,
			dialogue->transform.position.y + 64.0f + (float)i * 20.0f
		};
	}
}

void draw_dialogue() {
	if (!dialogue) {
		return;
	}
	const no::vector4f active{ 1.0f, 0.9f, 0.5f, 1.0f };
	const no::vector4f inactive{ 0.8f };
	dialogue->message_view.draw(shapes().rectangle);
	for (int i = 0; i < (int)dialogue->choice_views.size(); i++) {
		shaders().sprite.color.set(dialogue->current_choice == i ? active : inactive);
		dialogue->choice_views[i].draw(shapes().rectangle);
	}
}

no::message_event<int>& dialogue_choice_event() {
	return dialogue->events.choose;
}
