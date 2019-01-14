#pragma once

namespace no {

class timer {
public:

	void start();
	void stop();
	void pause();
	void resume();

	void go_back_in_time(long long milliseconds);

	bool is_paused() const;
	bool has_started() const;

	long long microseconds() const;
	long long milliseconds() const;
	long long seconds() const;
	long long minutes() const;
	long long hours() const;
	long long days() const;

private:

	long long microseconds_to_ticks(long long microseconds) const;
	long long ticks_to_microseconds(long long ticks) const;

	bool paused = false;
	bool started = false;

	long long paused_time = 0;
	long long start_time = 0;
	long long ticks_per_second = 0;

};

}
