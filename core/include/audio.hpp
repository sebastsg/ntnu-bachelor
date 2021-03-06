#pragma once

#include <cstdint>
#include <cstddef>

namespace no {

class io_stream;

enum class pcm_format { float_32, int_16, unknown };

class audio_source {
public:

	virtual ~audio_source() = default;

	virtual size_t size() const = 0;
	virtual pcm_format format() const = 0;
	virtual const io_stream& stream() const = 0;

};

class pcm_stream {
public:

	pcm_stream(audio_source* source);

	bool is_empty() const;
	float read_float();
	void reset();
	void stream(pcm_format format, uint8_t* destination, size_t size, size_t channels);
	void stream(float* destination, size_t count, size_t channels);

private:

	size_t position = 0;
	audio_source* source = nullptr;

};

class audio_player {
public:

	virtual ~audio_player() = default;

	virtual void play(const pcm_stream& stream) = 0;
	virtual void play(audio_source* source) = 0;
	virtual void pause() = 0;
	virtual void resume() = 0;
	virtual void stop() = 0;

	virtual bool is_playing() const = 0;
	virtual bool is_paused() const = 0;

};

class audio_endpoint {
public:

	virtual ~audio_endpoint() = default;

	virtual audio_player* add_player() = 0;
	virtual void stop_all_players() = 0;
	virtual void clear_players() = 0;

};

}
