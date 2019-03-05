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
	character_object player;
	game_variable_map variables;
	quest_instance_list quests;

Packet(other_player_joined, 1)
	character_object player;

Packet(player_disconnected, 2)
	int32_t player_instance_id = -1;

Packet(move_to_tile, 3)
	no::vector2i tile = -1;
	int32_t player_instance_id = -1;

Packet(chat_message, 4)
	std::string author;
	std::string message;

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
