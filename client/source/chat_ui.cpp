#include "chat_ui.hpp"
#include "game.hpp"
#include "unicode.hpp"

chat_view::chat_view(game_state& game, const no::ortho_camera& camera) : game(game), camera(camera) {
	enable();
}

chat_view::~chat_view() {
	disable();
}

void chat_view::update() {
	input.transform.position.xy = { 16.0f, camera.height() - input.transform.scale.y - 48.0f };
	const int lower = std::max(0, (int)history.size() - visible_history_length);
	float y = camera.height() - 96.0f;
	for (int i = (int)history.size() - 1; i >= lower; i--) {
		history[i].transform.position.xy = { input.transform.position.x + 8.0f, y };
		y -= history[i].transform.scale.y + 4.0f;
	}
}

void chat_view::draw() const {
	for (auto& message : history) {
		message.draw(rectangle);
	}
	input.draw(rectangle);
}

void chat_view::add(const std::string& author, const std::string& message) {
	history.emplace_back().render(game.font(), author + ": " + message);
}

void chat_view::enable() {
	key_press = game.keyboard().press.listen([this](const no::keyboard::press_message& event) {
		if (event.key == no::key::enter) {
			events.message.emit(input.text());
			add(game.player_name, input.text());
			input.render(game.font(), "");
		}
	});
	key_input = game.keyboard().input.listen([this](const no::keyboard::input_message& event) {
		if (event.character == (unsigned int)no::key::enter) {
			return;
		}
		if (event.character == (unsigned int)no::key::backspace) {
			if (!input.text().empty()) {
				input.render(game.font(), input.text().substr(0, input.text().size() - 1));
			}
		} else if (event.character < 0x7F) {
			input.render(game.font(), input.text() + (char)event.character);
		}
	});
}

void chat_view::disable() {
	game.keyboard().press.ignore(key_press);
	game.keyboard().input.ignore(key_input);
	key_press = -1;
	key_input = -1;
}
