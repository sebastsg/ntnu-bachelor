#pragma once

#include "windows_platform.hpp"
#include "audio.hpp"

#include <atomic>
#include <thread>
#include <vector>

struct IMMDevice;
struct IAudioClient;
struct IAudioRenderClient;
struct tWAVEFORMATEX;
using WAVEFORMATEX = tWAVEFORMATEX;

namespace no {

namespace wasapi {


class audio_client : public audio_player {
public:

	audio_client(IMMDevice* device);
	audio_client(const audio_client&) = delete;
	audio_client(audio_client&&) = delete;
	~audio_client() override;

	audio_client& operator=(const audio_client&) = delete;
	audio_client& operator=(audio_client&&) = delete;

	void play(const pcm_stream& stream) override;
	void play(audio_source* source) override;
	void pause() override;
	void resume() override;
	void stop() override;

	bool is_playing() const override;
	bool is_paused() const override;

private:

	void stream();
	void upload(unsigned int frames);

	std::thread thread;
	pcm_stream playing_audio_stream;

	std::atomic<bool> playing = false;
	std::atomic<bool> paused = false;

	IAudioClient* client = nullptr;
	IAudioRenderClient* render_client = nullptr;
	long long default_device_period = 0;
	unsigned int buffer_frame_count = 0;
	WAVEFORMATEX* wave_format = nullptr;

};

class audio_device : public audio_endpoint {
public:

	audio_device();
	~audio_device() override;

	audio_player* add_player() override;
	void stop_all_players() override;
	void clear_players() override;

private:

	IMMDevice* device = nullptr;
	std::vector<audio_client*> players;

};

}

}
