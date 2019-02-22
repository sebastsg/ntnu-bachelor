#include "loop.hpp"
#include "window.hpp"
#include "wasapi.hpp"
#include "windows_sockets.hpp"

#include "imgui.h"
#include "imgui_platform.h"

#include <ctime>

extern void configure();
extern void start();

namespace no {

frame_counter::frame_counter() {
	timer.start();
}

void frame_counter::next_frame() {
	delta_time = (double)(new_time - old_time) / 1000000.0;
	old_time = new_time;
	new_time = timer.microseconds();
	frames_this_second++;
	if (new_time - time_last_second >= 1000000) {
		time_last_second = new_time;
		frames_last_second = frames_this_second;
		frames_this_second = 0;
	}
	frame_count++;
}

long long frame_counter::ticks() const {
	return timer.microseconds();
}

long long frame_counter::frames() const {
	return frame_count;
}

long long frame_counter::current_fps() const {
	return frames_last_second;
}

double frame_counter::average_fps() const {
	return (double)frame_count / (double)ticks();
}

double frame_counter::delta() const {
	return delta_time;
}

static struct {

	int ticks_per_second = 60;
	int max_update_count = 5;
	frame_counter frame_counter;
	draw_synchronization synchronization = draw_synchronization::if_updated;

	audio_endpoint* audio = nullptr;

	std::vector<window_state*> states;
	std::vector<window*> windows;

	std::vector<window_state*> states_to_stop;

	long redundant_bind_calls_this_frame = 0;

	signal_event post_configure;

} loop;

signal_event& post_configure_event() {
	return loop.post_configure;
}

static int state_index(const window_state* state) {
	for (size_t i = 0; i < loop.states.size(); i++) {
		if (loop.states[i] == state) {
			return (int)i;
		}
	}
	return -1;
}

static void update_windows() {
	for (auto state : loop.states) {
		auto window = loop.windows[state_index(state)];
		window->poll();
		state->update();
	}
}

static void draw_windows() {
	for (auto state : loop.states) {
		auto window = loop.windows[state_index(state)];
		window->clear();
		state->draw();
		window->swap();
	}
}

static void destroy_stopped_states() {
	for (auto state : loop.states_to_stop) {
		int index = state_index(state);
		if (index != -1) {
			bool is_closing = !loop.states[index]->has_next_state();
			if (is_closing) {
				loop.audio->stop_all_players();
			}
			delete loop.states[index];
			loop.states.erase(loop.states.begin() + index);
			if (is_closing) {
				delete loop.windows[index];
				loop.windows.erase(loop.windows.begin() + index);
				loop.audio->clear_players();
			}
		}
	}
	loop.states_to_stop.clear();
}

window_state::window_state() {
	window_close_id = window().close.listen([this] {
		loop.states_to_stop.push_back(this);
	});
}

window_state::~window_state() {
	window().close.ignore(window_close_id);
	if (make_next_state) {
		loop.states.emplace_back(make_next_state());
	}
}

void window_state::stop() {
	loop.states_to_stop.push_back(this);
}

window& window_state::window() const {
	int index = state_index(this);
	if (index == -1) {
		// likely called from constructor. window would be added just before, so back() should be correct
		return *loop.windows.back();
	}
	return *loop.windows[index];
}

keyboard& window_state::keyboard() const {
	return window().keyboard;
}

mouse& window_state::mouse() const {
	return window().mouse;
}

frame_counter& window_state::frame_counter() {
	return loop.frame_counter;
}

void window_state::set_synchronization(draw_synchronization synchronization) {
	loop.synchronization = synchronization;
}

long window_state::redundant_bind_calls_this_frame() {
	return loop.redundant_bind_calls_this_frame;
}

void window_state::change_state(const internal::make_state_function& make_state) {
	make_next_state = make_state;
	loop.states_to_stop.push_back(this);
}

bool window_state::has_next_state() const {
	return make_next_state.operator bool();
}

std::string current_local_time_string() {
	time_t now = std::time(nullptr);
	tm local_time;
	localtime_s(&local_time, &now);
	char buffer[64];
	std::strftime(buffer, 64, "%X", &local_time);
	return buffer;
}

std::string curent_local_date_string() {
	time_t now = std::time(nullptr);
	tm local_time;
	localtime_s(&local_time, &now);
	char buffer[64];
	std::strftime(buffer, 64, "%Y.%m.%d", &local_time);
	return buffer;
}

namespace internal {

void create_state(const std::string& title, int width, int height, int samples, bool maximized, const make_state_function& make_state) {
	loop.windows.emplace_back(new window(title, width, height, samples, maximized));
	loop.states.emplace_back(make_state());
}

int run_main_loop() {
	configure();
	loop.post_configure.emit();

	loop.audio = new wasapi::audio_device();
	start_network();

	start();

	long long next_tick = loop.frame_counter.ticks();

	while (!loop.states.empty()) {
		int update_count = 0;
		long long frame_skip = 1000000 / loop.ticks_per_second;
		long long reference_ticks = loop.frame_counter.ticks();

		bool is_updated = false;
		while (reference_ticks - next_tick > frame_skip && update_count < loop.max_update_count) {
			update_windows();
			next_tick += frame_skip;
			update_count++;
			is_updated = true;
		}

		if (is_updated || loop.synchronization == draw_synchronization::always) {
			long current_redundant_bind_calls = total_redundant_bind_calls();
			draw_windows();
			loop.redundant_bind_calls_this_frame = total_redundant_bind_calls() - current_redundant_bind_calls;
			loop.frame_counter.next_frame();
		}

		destroy_stopped_states();
	}
	destroy_main_loop();
	return 0;
}

void destroy_main_loop() {
	for (auto state : loop.states) {
		loop.states_to_stop.push_back(state);
	}
	destroy_stopped_states();
	stop_network();
	delete loop.audio;
	loop.audio = nullptr;
}

}

}
