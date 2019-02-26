#include "packets.hpp"

#define Write(NAME)   void NAME::write(no::io_stream& stream) const { stream.write(type);
#define Read(NAME)  } void NAME::read(no::io_stream& stream) {
#define End         }

namespace to_client::game {

Write(player_joined)
	stream.write(is_me);
	player.write(stream);
Read(player_joined)
	is_me = stream.read<uint8_t>();
	player.read(stream);
End

Write(player_disconnected)
	stream.write(player_instance_id);
Read(player_disconnected)
	player_instance_id = stream.read<int32_t>();
End

Write(move_to_tile)
	stream.write(tile);
	stream.write(player_instance_id);
Read(move_to_tile)
	tile = stream.read<no::vector2i>();
	player_instance_id = stream.read<int32_t>();
End

Write(chat_message)
	stream.write(author);
	stream.write(message);
Read(chat_message)
	author = stream.read<std::string>();
	message = stream.read<std::string>();
End

}

namespace to_server::game {

Write(move_to_tile)
	stream.write(tile);
Read(move_to_tile)
	tile = stream.read<no::vector2i>();
End

Write(start_dialogue)
	stream.write(target_instance_id);
Read(start_dialogue)
	target_instance_id = stream.read<int32_t>();
End

Write(continue_dialogue)
	stream.write(choice);
Read(continue_dialogue)
	choice = stream.read<int32_t>();
End

Write(chat_message)
	stream.write(message);
Read(chat_message)
	message = stream.read<std::string>();
End

}

namespace to_client::lobby {

Write(login_status)
	stream.write(status);
	stream.write(name);
Read(login_status)
	status = stream.read<int32_t>();
	name = stream.read<std::string>();
End

}

namespace to_server::lobby {

Write(login_attempt)
	stream.write(name);
	stream.write(password);
Read(login_attempt)
	name = stream.read<std::string>();
	password = stream.read<std::string>();
End

Write(connect_to_world)
	stream.write(world);
Read(connect_to_world)
	world = stream.read<int32_t>();
End

}

namespace to_client::updates {

Write(latest_version)
	stream.write(version);
Read(latest_version)
	version = stream.read<int32_t>();
End

Write(file_transfer)
	stream.write(name);
	stream.write(file);
	stream.write(total_files);
	stream.write(offset);
	stream.write(total_size);
	stream.write_array<char>(data);
Read(file_transfer)
	name = stream.read<std::string>();
	file = stream.read<int32_t>();
	total_files = stream.read<int32_t>();
	offset = stream.read<int64_t>();
	total_size = stream.read<int64_t>();
	data = stream.read_array<char>();
End

}

namespace to_server::updates {

Write(update_query)
	stream.write(version);
	stream.write<int8_t>(needs_assets ? 1 : 0);
Read(update_query)
	version = stream.read<int32_t>();
	needs_assets = (stream.read<int8_t>() != 0);
End

}