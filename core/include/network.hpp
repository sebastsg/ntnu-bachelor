#pragma once

#include "event.hpp"
#include "io.hpp"
#include "debug.hpp"

#include <queue>

namespace no {

class socket_container;
class io_socket;

enum class ip_protocol { tcp, udp };
enum class address_family { inet4, inet6 };
enum class socket_close_status { disconnected_gracefully, connection_reset, not_connected, unknown };

void start_network();
void stop_network();

class packetizer {
public:

	static void start(io_stream& stream);
	static void end(io_stream& stream);

	packetizer();

	char* data();
	size_t write_index() const;
	void write(char* data, size_t size);
	io_stream next();
	void clean();

private:

	using magic_type = uint32_t;
	using body_size_type = uint32_t;

	static const magic_type magic = 'NFWK';
	static const size_t header_size = sizeof(magic_type) + sizeof(body_size_type);

	io_stream stream;

};

class abstract_socket {
public:

	int id = -1;

	bool disconnect();

	bool set_protocol(ip_protocol protocol);
	bool set_address_family(address_family family);

	// if we want to call asynchronous functions when the mutex is not locked, 
	// we must lock and unlock the mutex manually
	//
	// these functions must be used if you are trying to call another member function
	// outside of an event in the same namespace as the current class you are using
	//
	// examples:
	//
	// if this socket is an io_socket: 
	//     never call these when you handle an io_socket:: event
	//
	// if this socket is a listener_socket:
	//     never call these when you handle a listener_socket::event
	//
	// for all cases:
	//     always call these when you handle an event that is not in the socket's class
	//
	// usage: if (begin_async()) { ...; ...; end_async(); }
	bool begin_async();
	void end_async();

};

class socket_location {
public:

	// the index to access the socket through container
	size_t index = 0;
	socket_container* container = nullptr;

	// if we have no container, we must store the socket's this pointer
	io_socket* socket = nullptr;

	// quick way to get the most trustworthy pointer
	io_socket* get();

};

class io_socket : public abstract_socket {
public:

	struct disconnect_message {
		socket_close_status status = socket_close_status::unknown;
	};

	struct receive_stream_message {
		io_stream packet;
	};

	struct receive_packet_message {
		io_stream packet;
	};

	struct send_message {
		int placeholder = 0;
	};

	socket_location location;

	struct {
		message_event<receive_stream_message> receive_stream;
		message_event<receive_packet_message> receive_packet;
		message_event<send_message> send;
		message_event<disconnect_message> disconnect;
	} events;

	struct {
		event_message_queue<receive_stream_message> receive_stream;
		event_message_queue<receive_packet_message> receive_packet;
		event_message_queue<send_message> send;
		event_message_queue<disconnect_message> disconnect;
	} sync;

	io_socket();
	io_socket(const io_socket&) = delete;
	io_socket(io_socket&&);

	io_socket& operator=(const io_socket&) = delete;
	io_socket& operator=(io_socket&&);

	// if the i/o limit has been reached, this function will return false
	// if the send call itself failed, we also return false
	bool send(const io_stream& packet);

	// same as above, but wrapped around a begin_aync() and end_async()
	bool send_async(const io_stream& packet);

	// increment the number of asynchronous receive calls
	// if the I/O limit has been reached, this function will return false
	// if the receive call itself failed, we also return false
	bool receive();

	bool connect();
	bool connect(const std::string& address, int port);

	void synchronise();

};

class listener_socket : public abstract_socket {
public:

	struct accept_message {
		int error = 0;
		int id = -1;
	};

	struct {
		message_event<accept_message> accept;
	} events;

	bool bind(const std::string& address, int port);

	// bind the socket to a port
	bool bind();

	bool listen();

	// increment the number of asynchronous accept calls
	// if the I/O limit has been reached, this function will return false
	// if the accept call itself failed, we also return false
	bool accept();

	void synchronise();

	struct {
		event_message_queue<accept_message> accept;
	} sync;

};

template<typename T, size_t LIMIT>
class limited_queue {
public:

	T data[LIMIT];

	// the indices of data[] that are not currently used
	std::queue<size_t> available;

	// map the pointers to indices
	std::unordered_map<T*, size_t> indices;

	// try to fetch the next available data, nullptr if none
	T* create() {
		if (available.size() == 0) {
			return nullptr;
		}
		size_t index = available.front();
		available.pop();
		return &data[index];
	}

	void destroy(T* data) {
		*data = {};
		if (indices.find(data) == indices.end()) {
			WARNING(data << " does not belong to " << this);
			return;
		}
		// mark the data as available
		size_t index = indices[data];
		available.push(index);
	}

	limited_queue() {
		init();
	}

	limited_queue(const limited_queue& queue) {
		copy(queue);
	}

	limited_queue& operator=(const limited_queue& queue) {
		copy(queue);
		return *this;
	}

	limited_queue(limited_queue&& queue) {
		move(std::move(queue));
	}

	limited_queue& operator=(limited_queue&& queue) {
		move(std::move(queue));
		return *this;
	}

private:

	void copy(const limited_queue& that) {
		// copy the queue data, but make our own indices map
		for (size_t i = 0; i < LIMIT; i++) {
			data[i] = that.data[i];
			indices[&data[i]] = i;
		}
		available = that.available;
	}

	void move(limited_queue&& that) {
		// move the queue data, but make our own indices map
		for (size_t i = 0; i < LIMIT; i++) {
			data[i] = std::move(that.data[i]);
			indices[&data[i]] = i;
		}
		available = that.available;
		that.init();
	}

	void init() {
		// queue up the available data indices
		available = {};
		indices = {};
		for (size_t i = 0; i < LIMIT; i++) {
			available.push(i);
			indices[&data[i]] = i;
		}
	}

};

class socket_container {
public:

	struct receive_packet_message {
		size_t index;
		io_stream packet;
	};

	// all the sockets contained. not guaranteed to be active
	std::vector<io_socket> sockets;

	// which indices in the sockets vector are currently unused, and can be reused
	std::queue<size_t> available_sockets;

	// which indices in the sockets vector are currently in use
	std::vector<size_t> active_sockets;

	template<bool Sync>
	void broadcast(const io_stream& stream) {
		if constexpr (Sync) {
			for (size_t i : active_sockets) {
				sockets[i].send(stream);
			}
		} else {
			for (size_t i : active_sockets) {
				sockets[i].send_async(stream);
			}
		}
	}

	template<bool Sync>
	void broadcast(const io_stream& stream, size_t except_index) {
		if constexpr (Sync) {
			for (size_t i : active_sockets) {
				if (i != except_index) {
					sockets[i].send(stream);
				}
			}
		} else {
			for (size_t i : active_sockets) {
				if (i != except_index) {
					sockets[i].send_async(stream);
				}
			}
		}
	}

	io_socket& operator[](size_t index) {
		return sockets[index];
	}

	const io_socket& operator[](size_t index) const {
		return sockets[index];
	}

	// create a new active (not open) socket, and get its index as a return value
	size_t create();

	// destroy an active socket
	void destroy(size_t index);

	void synchronise();

};

class connection_establisher {
public:

	struct established_message {
		size_t index = 0;
	};

	listener_socket listener;
	socket_container* container = nullptr;

	struct {
		message_event<established_message> established;
	} events;

	void listen(const std::string& host, int port);

	void synchronise();

private:

	struct {
		event_message_queue<listener_socket::accept_message> accept;
	} sync;

};

template<typename P>
io_stream packet_stream(const P& packet) {
	io_stream stream;
	packetizer::start(stream);
	packet.write(stream);
	packetizer::end(stream);
	return stream;
}

}

std::ostream& operator<<(std::ostream& out, no::socket_close_status status);
std::ostream& operator<<(std::ostream& out, no::address_family family);
