#pragma once

#include "debug.hpp"

#include <functional>
#include <vector>
#include <queue>

namespace no {

class signal_event {
public:

	using handler = std::function<void()>;

	int listen(const handler& handler);
	void emit() const;
	void ignore(int id);

	int listeners() const;

private:

	std::vector<handler> handlers;

};

template<typename M>
class message_event {
public:

	using handler = std::function<void(const M&)>;

	int listen(const handler& handler) {
		if (!handler) {
			return -1;
		}
		for (size_t i = 0; i < handlers.size(); i++) {
			if (!handlers[i]) {
				handlers[i] = handler;
				return (int)i;
			}
		}
		handlers.push_back(handler);
		return (int)handlers.size() - 1;
	}

	void emit(const M& event) const {
		for (auto& handler : handlers) {
			if (handler) {
				handler(event);
			}
		}
	}

	void ignore(int id) {
		if (id >= 0 && id < (int)handlers.size()) {
			handlers[id] = {};
		}
	}

	int listeners() const {
		return (int)handlers.size();
	}

private:

	std::vector<handler> handlers;

};

template<typename M>
class event_message_queue {
public:

	event_message_queue() = default;
	event_message_queue(const event_message_queue&) = delete;
	event_message_queue(event_message_queue&& that) : messages(std::move(that.messages)) {}

	event_message_queue& operator=(const event_message_queue&) = delete;
	event_message_queue& operator=(event_message_queue&& that) {
		std::swap(messages, that.messages);
		return *this;
	}

	void move_and_push(M&& message) {
		messages.push(std::move(message));
	}

	template<typename... Args>
	void emplace_and_push(Args... args) {
		messages.emplace(M{ std::forward<Args>(args)... });
	}

	void all(const std::function<void(const M&)>& function) {
		if (function) {
			while (!messages.empty()) {
				function(messages.front());
				messages.pop();
			}
		}
	}

	void emit(const message_event<M>& listener) {
		while (!messages.empty()) {
			listener.emit(messages.front());
			messages.pop();
		}
	}

	size_t size() const {
		return messages.size();
	}

private:

	std::queue<M> messages;

};

}
