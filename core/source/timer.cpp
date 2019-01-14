#include "timer.hpp"
#include "platform.hpp"

namespace no {

void timer::start() {
	started = true;
	paused = false;
	paused_time = 0;
	start_time = platform::performance_counter();
	ticks_per_second = platform::performance_frequency();
}

void timer::stop() {
	started = false;
	paused = false;
	paused_time = 0;
	start_time = 0;
	ticks_per_second = 0;
}

void timer::pause() {
	if (started && !paused) {
		paused = true;
		long long now = platform::performance_counter();
		paused_time = ticks_to_microseconds(now - start_time);
	}
}

void timer::resume() {
	paused = false;
	paused_time = 0;
}

void timer::go_back_in_time(long long milliseconds) {
	if (!started) {
		return;
	}
	if (paused) {
		paused_time -= microseconds_to_ticks(milliseconds * 1000);
	} else {
		start_time -= microseconds_to_ticks(milliseconds * 1000);
	}
}

bool timer::is_paused() const {
	return paused;
}

bool timer::has_started() const {
	return started;
}

long long timer::microseconds() const {
	if (!started) {
		return 0;
	}
	if (paused) {
		return paused_time;
	}
	long long now = platform::performance_counter();
	return ticks_to_microseconds(now - start_time);
}

long long timer::milliseconds() const {
	return microseconds() / 1000;
}

long long timer::seconds() const {
	return microseconds() / 1000 / 1000;
}

long long timer::minutes() const {
	return microseconds() / 1000 / 1000 / 60;
}

long long timer::hours() const {
	return microseconds() / 1000 / 1000 / 60 / 60;
}

long long timer::days() const {
	return microseconds() / 1000 / 1000 / 60 / 60 / 24;
}

long long timer::microseconds_to_ticks(long long microseconds) const {
	return microseconds * ticks_per_second / 1000000;
}

long long timer::ticks_to_microseconds(long long ticks) const {
	return ticks * 1000000 / ticks_per_second;
}

}
