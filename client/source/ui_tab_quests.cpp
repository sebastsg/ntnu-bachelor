#include "ui_tab_quests.hpp"
#include "ui_tabs.hpp"
#include "game.hpp"
#include "game_assets.hpp"
#include "ui.hpp"

static const no::vector4f quest_item_uv{ 698.0f, 545.0f, 138.0f, 20.0f };

struct quests_view {

	game_state& game;
	std::list<no::button> buttons;
	int cursor_icon_id = -1;

	quests_view(game_state& game_) : game{ game_ } {
		for (int i = 0; i < quest_definitions().count(); i++) {
			auto quest = quest_definitions().find_by_index(i);
			auto& button = buttons.emplace_back(game, game.ui_camera_2x);
			button.set_tex_coords(quest_item_uv.xy / 1024.0f, quest_item_uv.zw / 1024.0f);
			button.transition.enabled = false;
			button.animation.frames = 1;
			button.label_alignment = no::align_type::left;
			button.label_padding.x = 4.0f;
			button.label.render(fonts().leo_9, quest->name);
			button.events.click.listen([this] {

			});
		}
		cursor_icon_id = game.mouse().icon.listen([this] {
			for (auto& button : buttons) {
				if (button.is_hovered()) {
					game.mouse().set_icon(no::mouse::cursor::pointer);
					break;
				}
			}
		});
	}

	~quests_view() {
		game.mouse().icon.ignore(cursor_icon_id);
	}

};

static quests_view* quests = nullptr;

void show_quests_tab(game_state& game) {
	hide_quests_tab();
	quests = new quests_view{ game };
}

void hide_quests_tab() {
	delete quests;
	quests = nullptr;
}

void update_quests_tab() {
	if (!quests) {
		return;
	}
	float y = 0.0f;
	for (auto& button : quests->buttons) {
		button.transform.position = tab_body_transform().position;
		button.transform.position.x += 3.0f;
		button.transform.position.y += 6.0f + y;
		button.transform.scale = quest_item_uv.zw;
		button.update();
		y += quest_item_uv.w;
	}
}

void draw_quests_tab() {
	if (!quests) {
		return;
	}
	for (auto& button : quests->buttons) {
		button.color = shaders().sprite.color;
		if (button.is_hovered()) {
			if (button.is_pressed()) {
				shaders().sprite.color.set({ 1.0f, 0.2f, 0.2f, 1.0f });
			} else {
				shaders().sprite.color.set({ 1.0f, 0.7f, 0.3f, 1.0f });
			}
		} else {
			shaders().sprite.color.set(no::vector4f{ 1.0f });
		}
		no::bind_texture(sprites().ui);
		button.draw_button();
		shaders().sprite.color.set(no::vector4f{ 1.0f });
		button.draw_label();
	}
	shaders().sprite.color.set(no::vector4f{ 1.0f });
}

void add_quests_context_menu_options() {

}
