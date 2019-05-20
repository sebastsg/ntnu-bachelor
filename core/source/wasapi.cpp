#include "platform.hpp"

#if ENABLE_WASAPI

#include "wasapi.hpp"
#include "debug.hpp"
#include "io.hpp"
#include "audio.hpp"
#include "ogg_vorbis.hpp"

#include <mmdeviceapi.h>
#include <Audioclient.h>

namespace no {

namespace wasapi {

namespace device_enumerator {

static IMMDeviceEnumerator* device_enumerator = nullptr;

static void create() {
	if (device_enumerator) {
		return;
	}
	HRESULT result = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&device_enumerator);
	if (result != S_OK) {
		WARNING("Failed to create device enumerator. Error: " << std::hex << result);
	}
}

static void release() {
	if (device_enumerator) {
		device_enumerator->Release();
		device_enumerator = nullptr;
	}
}

static auto get() {
	return device_enumerator;
}

}

static pcm_format wave_format_to_audio_data_format(WAVEFORMATEX* wave_format) {
	auto extendedFormat = (WAVEFORMATEXTENSIBLE*)wave_format;
	switch (wave_format->wFormatTag) {
	case WAVE_FORMAT_IEEE_FLOAT:
		return pcm_format::float_32;
	case WAVE_FORMAT_PCM:
		return pcm_format::int_16;
	case WAVE_FORMAT_EXTENSIBLE:
		if (memcmp(&extendedFormat->SubFormat, &KSDATAFORMAT_SUBTYPE_IEEE_FLOAT, sizeof(GUID)) == 0) {
			return pcm_format::float_32;
		}
		if (memcmp(&extendedFormat->SubFormat, &KSDATAFORMAT_SUBTYPE_PCM, sizeof(GUID)) == 0) {
			return pcm_format::int_16;
		}
		return pcm_format::unknown;
	default:
		WARNING("Unknown wave format: " << wave_format->wFormatTag);
		return pcm_format::unknown;
	}
}

audio_client::audio_client(IMMDevice* device) : playing_audio_stream(nullptr) {
	HRESULT result = device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)&client);
	if (result != S_OK) {
		WARNING("Failed to activate audio client. Error: " << result);
		return;
	}
	result = client->GetMixFormat(&wave_format);
	if (result != S_OK) {
		WARNING("Failed to get mix format. Error : " << result);
		return;
	}
	result = client->GetDevicePeriod(&default_device_period, nullptr);
	if (result != S_OK) {
		WARNING("Failed to get device period. Error : " << result);
		return;
	}
	result = client->Initialize(AUDCLNT_SHAREMODE_SHARED, 0, default_device_period, 0, wave_format, nullptr);
	if (result != S_OK) {
		WARNING("Failed to initialize the audio client. Error: " << result);
		return;
	}
	result = client->GetBufferSize(&buffer_frame_count);
	if (result != S_OK) {
		WARNING("Failed to get buffer size. Error: " << result);
		return;
	}
	result = client->GetService(__uuidof(IAudioRenderClient), (void**)&render_client);
	if (result != S_OK) {
		WARNING("Failed to get audio render client. Error: " << result);
		return;
	}
	INFO("-- [b]WASAPI Audio Client[/b] --"
		 << "\n[b]Device Period:[/b] " << default_device_period
		 << "\n[b]Buffer Frame Count:[/b] " << buffer_frame_count
	);
}

audio_client::~audio_client() {
	stop();
	CoTaskMemFree(wave_format);
	if (client) {
		client->Release();
	}
	if (render_client) {
		render_client->Release();
	}
}

void audio_client::stream() {
	if (playing_audio_stream.is_empty()) {
		WARNING("Audio source is empty.");
		return;
	}
	upload(buffer_frame_count);
	HRESULT result = client->Start();
	if (result != S_OK) {
		WARNING_X(1, "Failed to play audio. Error: " << result);
		return;
	}
	const long long one_millisecond = 10000;
	const long long one_second = 10000000;
	const DWORD sleep_period_ms = (DWORD)(default_device_period / one_millisecond / 2);
	bool has_been_stopped = false;
	while (true) {
		if (!playing) {
			client->Stop();
			return;
		}
		if (playing_audio_stream.is_empty()) {
			break;
		}
		Sleep(sleep_period_ms);
		if (paused) {
			if (!has_been_stopped) {
				has_been_stopped = true;
				client->Stop();
			}
			continue;
		}
		if (has_been_stopped) {
			has_been_stopped = false;
			client->Start();
		}
		UINT32 padding_frames = 0;
		result = client->GetCurrentPadding(&padding_frames);
		if (result != S_OK) {
			WARNING_X(1, "Failed to get padding. Error: " << result);
			break;
		}
		int frames_available = buffer_frame_count - padding_frames;
		upload(frames_available);
	}
	Sleep(sleep_period_ms);
	client->Stop();
}

void audio_client::play(const pcm_stream& new_audio_stream) {
	stop();
	playing = true;
	playing_audio_stream = new_audio_stream;
	thread = std::thread{ &audio_client::stream, this };
}

void audio_client::play(audio_source* source) {
	play(pcm_stream{ source });
}

void audio_client::pause() {
	paused = true;
}

void audio_client::resume() {
	paused = false;
}

bool audio_client::is_playing() const {
	return playing;
}

bool audio_client::is_paused() const {
	return paused;
}

void audio_client::stop() {
	playing = false;
	if (thread.joinable()) {
		thread.join();
	}
	playing_audio_stream.reset();
	HRESULT result = client->Reset();
	if (result != S_OK) {
		WARNING("Failed to reset the audio client. Error: " << result);
	}
	paused = false;
}

void audio_client::upload(unsigned int frames) {
	BYTE* buffer = nullptr;
	HRESULT result = render_client->GetBuffer(frames, &buffer);
	if (result != S_OK) {
		WARNING_X(1, "Failed to get buffer. Error: " << result);
		return;
	}

	auto format = wave_format_to_audio_data_format(wave_format);
	size_t size = frames * wave_format->nBlockAlign;
	size_t channels = wave_format->nChannels;
	playing_audio_stream.stream(format, buffer, size, channels);

	result = render_client->ReleaseBuffer(frames, 0);
	if (result != S_OK) {
		WARNING_X(1, "Failed to release buffer. Error: " << result);
	}
}

audio_device::audio_device() {
	MESSAGE("Initializing audio device");
	device_enumerator::create();
	HRESULT result = device_enumerator::get()->GetDefaultAudioEndpoint(eRender, eConsole, &device);
	if (result != S_OK) {
		WARNING("Failed to get default audio endpoint. Error: " << result);
	}
}

audio_device::~audio_device() {
	clear_players();
	if (device) {
		device->Release();
	}
	device_enumerator::release();
}

audio_player* audio_device::add_player() {
	MESSAGE("Adding audio player");
	players.push_back(new audio_client(device));
	return players.back();
}

void audio_device::stop_all_players() {
	MESSAGE("Stopping all audio players");
	for (auto& player : players) {
		player->stop();
	}
}

void audio_device::clear_players() {
	MESSAGE("Clearing all audio players");
	for (auto& player : players) {
		delete player;
	}
	players.clear();
}

}

}

#endif
