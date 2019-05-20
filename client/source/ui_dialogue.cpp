#include "ui_dialogue.hpp"
#include "ui_tabs.hpp"
#include "game.hpp"
#include "game_assets.hpp"
#include "script.hpp"
#include "ui.hpp"

static const no::vector4f dialogue_background_uv{ 34.0f, 868.0f, 358.0f, 80.0f };
static const no::vector4f dialogue_choice_uv{ 60.0f, 959.0f, 306.0f, 25.0f };

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

	no::rectangle background_rectangle;
	no::rectangle choice_rectangle;
	no::text_view message_view;
	std::vector<no::text_view> choice_views;
	std::vector<no::transform2> choice_transforms;

	dialogue_view(game_state& game) : game{ game }, message_view{ game, game.ui_camera_2x } {
		set_ui_uv(background_rectangle, dialogue_background_uv);
		set_ui_uv(choice_rectangle, dialogue_choice_uv);
	}

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
			dialogue->choice_views.emplace_back(dialogue->game, dialogue->game.ui_camera_2x).render(fonts().leo_10, choice.text);
			dialogue->choice_transforms.emplace_back();
		}
	});
	tree.process_entry_point();
	dialogue->key_listener = game.keyboard().press.listen([&tree](no::key pressed_key) {
		// todo: 1, 2, 3, 4, 5... etc to select choice
		switch (pressed_key) {
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
	dialogue->transform.scale = dialogue_background_uv.zw;
	dialogue->transform.position = dialogue->game.ui_camera_2x.size() / 2.0f - dialogue->transform.scale / 2.0f;
	dialogue->message_view.transform.position = dialogue->transform.position + 32.0f;
	for (int i = 0; i < (int)dialogue->choice_views.size(); i++) {
		dialogue->choice_transforms[i].position.x = dialogue->transform.position.x + 26.0f;
		dialogue->choice_transforms[i].position.y = dialogue->transform.position.y - 2.0f + dialogue_background_uv.w + (float)i * dialogue_choice_uv.w;
		dialogue->choice_transforms[i].scale = dialogue_choice_uv.zw;
		dialogue->choice_views[i].transform.position = {
			dialogue->choice_transforms[i].position.x + 6.0f,
			dialogue->choice_transforms[i].position.y + 8.0f
		};
	}
}

void draw_dialogue() {
	if (!dialogue) {
		return;
	}
	no::bind_texture(sprites().ui);
	no::draw_shape(dialogue->background_rectangle, dialogue->transform);
	
	const no::vector4f active{ 1.0f, 0.9f, 0.5f, 1.0f };
	const no::vector4f inactive{ 0.8f };
	
	dialogue->message_view.draw(shapes().rectangle);
	for (int i = 0; i < (int)dialogue->choice_views.size(); i++) {
		shaders().sprite.color.set(dialogue->current_choice == i ? active : inactive);
		no::bind_texture(sprites().ui);
		no::draw_shape(dialogue->choice_rectangle, dialogue->choice_transforms[i]);
		dialogue->choice_views[i].draw(shapes().rectangle);
	}
}

no::message_event<int>& dialogue_choice_event() {
	return dialogue->events.choose;
}
