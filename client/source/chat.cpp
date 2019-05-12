#include "chat.hpp"
#include "game.hpp"
#include "unicode.hpp"
#include "game_assets.hpp"
#include "ui.hpp"

struct chat_view {

	struct {
		no::message_event<std::string> message;
	} events;

	game_state& game;
	int key_press = -1;
	int key_input = -1;
	no::text_view input;
	std::vector<no::text_view> history;
	int visible_history_length = 10;

	chat_view(game_state& game) : game{ game }, input{ game, game.ui_camera_2x } {}

};

static chat_view* chat = nullptr;

void show_chat(game_state& game) {
	hide_chat();
	chat = new chat_view{ game };
}

void hide_chat() {
	if (chat) {
		disable_chat();
		delete chat;
		chat = nullptr;
	}
}

void update_chat() {
	if (!chat) {
		return;
	}
	chat->input.transform.position = { 16.0f, chat->game.ui_camera_2x.height() - chat->input.transform.scale.y - 48.0f };
	const int lower = std::max(0, (int)chat->history.size() - chat->visible_history_length);
	float y = chat->game.ui_camera_2x.height() - 96.0f;
	for (int i = (int)chat->history.size() - 1; i >= lower; i--) {
		chat->history[i].transform.position = { chat->input.transform.position.x + 8.0f, y };
		y -= chat->history[i].transform.scale.y + 4.0f;
	}
}

void draw_chat() {
	if (!chat) {
		return;
	}
	for (auto& message : chat->history) {
		message.draw(shapes().rectangle);
	}
	chat->input.draw(shapes().rectangle);
}

void add_chat_message(const std::string& author, const std::string& message) {
	std::string text = (author.empty() ? "" : (author + ": ")) + message;
	chat->history.emplace_back(chat->game, chat->game.ui_camera_2x).render(fonts().leo_14, text);
}

void enable_chat() {
	chat->key_press = chat->game.keyboard().press.listen([](no::key pressed_key) {
		if (pressed_key == no::key::enter && !chat->input.text().empty()) {
			chat->events.message.emit(chat->input.text());
			add_chat_message(chat->game.player_name(), chat->input.text());
			chat->input.render(fonts().leo_14, "");
		}
	});
	chat->key_input = chat->game.keyboard().input.listen([](unsigned int character) {
		if (character == (unsigned int)no::key::enter) {
			return;
		}
		if (character == (unsigned int)no::key::backspace) {
			if (!chat->input.text().empty()) {
				chat->input.render(fonts().leo_14, chat->input.text().substr(0, chat->input.text().size() - 1));
			}
		} else if (character < 0x7F) {
			chat->input.render(fonts().leo_14, chat->input.text() + (char)character);
		}
	});
}

void disable_chat() {
	chat->game.keyboard().press.ignore(chat->key_press);
	chat->game.keyboard().input.ignore(chat->key_input);
	chat->key_press = -1;
	chat->key_input = -1;
}

no::message_event<std::string>& chat_message_event() {
	return chat->events.message;
}
