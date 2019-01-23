#include "server.hpp"
#include "debug.hpp"
#include "io.hpp"
#include "platform.hpp"
#include "assets.hpp"
#include "packets.hpp"

server_world::server_world(const std::string& name) {
	no::file::read(no::asset_path("worlds/" + name + ".ew"), stream);
}

server_world::server_world(server_world&& that) : stream(std::move(that.stream)) {

}

server_state::server_state() {
	worlds.emplace_back("main");
	
	establisher.container = &sockets;
	establisher.listen("10.0.0.130", 7524); // todo: config file
	establisher.events.established.listen([this](const no::connection_establisher::established_message& event) {
		connect((int)event.index);
	});
}

server_state::~server_state() {
	
}

void server_state::update() {
	establisher.synchronise();
	sockets.synchronise();
}

void server_state::draw() {
	
}

void server_state::connect(int index) {
	if (index >= max_clients) {
		// todo: send some message to client to let it know properly
		WARNING("Rejecting client. Connection limit reached");
		return;
	}
	if (clients[index].is_connected()) {
		// todo: check if this can happen
		WARNING("Rejecting client. This slot is already in use.");
		return;
	}
	INFO("Connecting client " << index);
	clients[index] = { true };

	session_packet session;
	session.is_me = 1;
	session.player_id = index;
	session.tile_x = clients[index].tile_x;
	session.tile_z = clients[index].tile_z;
	no::io_stream session_stream;
	no::packetizer::start(session_stream);
	session.write(session_stream);
	no::packetizer::end(session_stream);
	sockets[index].send_async(session_stream);

	for (int j = 0; j < max_clients; j++) {
		if (clients[j].is_connected() && index != j) {
			session.is_me = 0;
			session.player_id = j;
			session.tile_x = clients[j].tile_x;
			session.tile_z = clients[j].tile_z;
			session_stream = {};
			no::packetizer::start(session_stream);
			session.write(session_stream);
			no::packetizer::end(session_stream);
			sockets[index].send_async(session_stream);
		}
	}

	session_stream = {};
	session.player_id = index;
	session.is_me = 0;
	session.tile_x = clients[index].tile_x;
	session.tile_z = clients[index].tile_z;
	no::packetizer::start(session_stream);
	session.write(session_stream);
	no::packetizer::end(session_stream);
	sockets.broadcast<false>(session_stream, index);

	sockets[index].events.receive_packet.listen([this, index](const no::io_socket::receive_packet_message& event) {
		no::io_stream stream = { event.packet.data(), event.packet.size(), no::io_stream::construct_by::shallow_copy };
		int16_t type = stream.read<int16_t>();
		switch (type) {
		case move_to_tile_packet::type:
		{
			move_to_tile_packet packet;
			packet.read(stream);

			packet.player_id = index;

			clients[index].tile_x = packet.tile_x;
			clients[index].tile_z = packet.tile_z;

			no::io_stream packet_stream;
			no::packetizer::start(packet_stream);
			packet.write(packet_stream);
			no::packetizer::end(packet_stream);
			sockets.broadcast<true>(packet_stream);
			break;
		}
		default:
			break;
		}
	});

	sockets[index].events.disconnect.listen([this, index](const no::io_socket::disconnect_message& event) {
		INFO("Client " << index << " has disconnected with status " << event.status);
		clients[index] = { false };

		no::io_stream disconnect_stream;
		player_disconnected_packet disconnection;
		disconnection.player_id = index;
		no::packetizer::start(disconnect_stream);
		disconnection.write(disconnect_stream);
		no::packetizer::end(disconnect_stream);
		sockets.broadcast<true>(disconnect_stream);
	});
}

void server_state::disconnect(int index) {
	if (index >= max_clients) {
		return;
	}
	clients[index] = { false };
	INFO("Disconnecting client " << index);
}

void configure() {
#if _DEBUG
	no::set_asset_directory("../../../assets");
#else
	no::set_asset_directory("../../../assets");
	//no::set_asset_directory("assets");
#endif
}

void start() {
	no::create_state<server_state>("Server", 800, 600, 4, false);
}
