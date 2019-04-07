#pragma once

#include "character.hpp"
#include "gamevar.hpp"
#include "quest.hpp"
#include "io.hpp"
#include "math.hpp"

// undefined at end of file.
#define Begin(SCOPE, TYPE) namespace SCOPE { constexpr uint16_t scope_type = TYPE;
#define Packet1(NAME, TYPE) \
struct NAME { \
    NAME() = default; \
    NAME(no::io_stream& stream) { read(stream); } \
    static const uint16_t type = scope_type + TYPE; \
    void write(no::io_stream& stream) const; \
    void read(no::io_stream& stream);
#define End }; }
#define Packet(NAME, TYPE) }; Packet1(NAME, TYPE)

Begin(to_client::game, 0)

Packet1(my_player_info, 0)
	character_object player = { -1 };
	game_object object;
	game_variable_map variables;
	quest_instance_list quests;

Packet(other_player_joined, 1)
	character_object player = { -1 };
	game_object object;

Packet(player_disconnected, 2)
	int32_t player_instance_id = -1;

Packet(move_to_tile, 3)
	no::vector2i tile = -1;
	int32_t player_instance_id = -1;

Packet(chat_message, 4)
	std::string author;
	std::string message;

Packet(combat_hit, 5)
	int32_t attacker_id = -1;
	int32_t target_id = -1;
	int32_t damage = 0;

Packet(character_equips, 6)
	int32_t instance_id = -1;
	int32_t item_id = -1;
	int64_t stack = 0;

Packet(character_unequips, 7)
	int32_t instance_id = -1;
	equipment_slot slot = equipment_slot::none;

Packet(character_follows, 8)
	int32_t follower_id = -1;
	int32_t target_id = -1;

Packet(trade_request, 9)
	int32_t trader_id = -1;

Packet(add_trade_item, 10)
	item_instance item;

Packet(remove_trade_item, 11)
	no::vector2i slot;
	
Packet(trade_decision, 12)
	bool accepted = false;

Packet(started_fishing, 13)
	int32_t instance_id = -1;
	no::vector2i casted_to_tile;

Packet(fishing_progress, 14)
	int32_t instance_id = -1;
	no::vector2i new_bait_tile;
	bool finished = false;

Packet(fish_caught, 15)
	item_instance item;

End

Begin(to_server::game, 1000)

Packet1(move_to_tile, 0)
	no::vector2i tile = -1;

Packet(start_dialogue, 1)
	int32_t target_instance_id = -1;

Packet(continue_dialogue, 2)
	int32_t choice = -1;

Packet(chat_message, 3)
	std::string message;

Packet(start_combat, 4)
	int32_t target_id = -1;

Packet(equip_from_inventory, 5)
	no::vector2i slot;

Packet(unequip_to_inventory, 6)
	equipment_slot slot = equipment_slot::none;

Packet(follow_character, 7)
	int32_t target_id = -1;

Packet(trade_request, 8)
	int32_t trade_with_id = -1;

Packet(add_trade_item, 9)
	item_instance item;

Packet(remove_trade_item, 10)
	no::vector2i slot;
	
Packet(trade_decision, 11)
	bool accepted = false;

Packet(started_fishing, 12)
	no::vector2i casted_to_tile;

End

Begin(to_client::lobby, 2000)

Packet1(login_status, 0)
	int32_t status = 0;
	std::string name;

End

Begin(to_server::lobby, 3000)

Packet1(login_attempt, 0)
	std::string name;
	std::string password;

Packet(connect_to_world, 1)
	int32_t world = 0;

End

Begin(to_client::updates, 4000)

Packet1(latest_version, 0)
	int32_t version = 0;

Packet(file_transfer, 1)
	std::string name;
	int32_t file = 0;
	int32_t total_files = 0;
	int64_t offset = 0;
	int64_t total_size = 0;
	std::vector<char> data;

End

Begin(to_server::updates, 5000)

Packet1(update_query, 0)
	int32_t version = 0;
	bool needs_assets = false;

End

#undef Begin
#undef Packet1
#undef Packet
#undef End
