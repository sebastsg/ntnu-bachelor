#include "audio.hpp"
#include "io.hpp"
#include "debug.hpp"

#include <climits>

namespace no {

pcm_stream::pcm_stream(audio_source* source) : source(source) {

}

bool pcm_stream::is_empty() const { // todo: check format()
	return position + sizeof(int16_t) >= source->size();
}

float pcm_stream::read_float() {
	if (is_empty()) {
		return 0.0f;
	}
	int16_t pcm = source->stream().read<int16_t>(position); // todo: check format()
	position += sizeof(int16_t);
	return (float)((double)pcm / (double)SHRT_MAX);
}

void pcm_stream::reset() {
	position = 0;
}

void pcm_stream::stream(pcm_format format, uint8_t* destination, size_t size, size_t channels) {
	switch (format) {
	case pcm_format::float_32:
		stream((float*)destination, size / sizeof(float), channels);
		break;
	default:
		ASSERT(false);
		break;
	}
}

void pcm_stream::stream(float* destination, size_t count, size_t channels) {
	for (size_t i = 0; i < count; i += channels) {
		for (size_t j = 0; j < channels; j++) {
			destination[i + j] = read_float();
		}
	}
}

}
