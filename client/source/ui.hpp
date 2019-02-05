#pragma once

#include "world.hpp"
#include "draw.hpp"

class user_interface_view {
public:

	no::ortho_camera camera;

	user_interface_view(world_state& world);
	user_interface_view(const user_interface_view&) = delete;
	user_interface_view(user_interface_view&&) = delete;

	~user_interface_view();

	user_interface_view& operator=(const user_interface_view&) = delete;
	user_interface_view& operator=(user_interface_view&&) = delete;

	void listen(player_object* player);
	void ignore();

	void draw() const;

private:

	static constexpr no::vector2f item_size = 32.0f;
	static constexpr no::vector2f item_grid = item_size + 2.0f;
	static constexpr no::vector4f inventory_uv = { 392.0f, 48.0f, 184.0f, 352.0f };

	void set_ui_uv(no::rectangle& rectangle, no::vector4f uv);
	void set_item_uv(no::rectangle& rectangle, no::vector2f uv);

	world_state& world;

	int ui_texture = -1;

	struct inventory_slot {
		no::rectangle rectangle;
		item_instance item;
	};

	struct {
		no::rectangle background;
		std::unordered_map<int, inventory_slot> slots;
		int add_item_event = -1;
		int remove_item_event = -1;
	} inventory;

	int shader = -1;
	player_object* player = nullptr;
	int equipment_event = -1;

};
