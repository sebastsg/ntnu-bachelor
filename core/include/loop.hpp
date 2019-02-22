#pragma once

#include "timer.hpp"
#include "event.hpp"

#include <string>
#include <functional>

namespace no {

class window;
class window_state;
class keyboard;
class mouse;

namespace internal {

using make_state_function = std::function<window_state*()>;

void create_state(const std::string& title, int width, int height, int samples, bool maximized, const make_state_function& make_state);
int run_main_loop();
void destroy_main_loop();

}

class frame_counter {
public:

	frame_counter();

	void next_frame();

	long long ticks() const;
	long long frames() const;
	long long current_fps() const;
	double average_fps() const;
	double delta() const;

private:

	timer timer;
	long long frame_count = 0;
	long long old_time = 0;
	long long new_time = 0;
	long long time_last_second = 0;
	long long frames_this_second = 0;
	long long frames_last_second = 0;
	double delta_time = 0.0;

};

// 'always' should only used to test performance, and requires swap_interval::immediate.
enum class draw_synchronization { always, if_updated };

class window_state {
public:

	window_state();
	window_state(const window_state&) = delete;
	window_state(window_state&&) = delete;

	virtual ~window_state();

	window_state& operator=(const window_state&) = delete;
	window_state& operator=(window_state&&) = delete;

	virtual void update() = 0;
	virtual void draw() = 0;

	window& window() const;
	keyboard& keyboard() const;
	mouse& mouse() const;

	bool has_next_state() const;

protected:

	template<typename T>
	void change_state() {
		change_state([] {
			return new T();
		});
	}

	frame_counter& frame_counter();
	void set_synchronization(draw_synchronization synchronization);
	long redundant_bind_calls_this_frame();

	void stop();

private:

	void change_state(const internal::make_state_function& make_state);

	internal::make_state_function make_next_state;
	int window_close_id = -1;

};

signal_event& post_configure_event();

template<typename T>
void create_state(const std::string& title, int width, int height, int samples, bool maximized) {
	internal::create_state(title, width, height, samples, maximized, [] {
		return new T();
	});
}

std::string current_local_time_string();
std::string curent_local_date_string();

}
