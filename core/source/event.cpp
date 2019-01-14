#include "event.hpp"

namespace no {

int signal_event::listen(const handler& handler) {
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

void signal_event::emit() const {
	for (auto& handler : handlers) {
		if (handler) {
			handler();
		}
	}
}

void signal_event::ignore(int id) {
	if (id >= 0 && id < (int)handlers.size()) {
		handlers[id] = {};
	}
}

int signal_event::listeners() const {
	return (int)handlers.size();
}

}
