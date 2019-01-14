#include "ogg_vorbis.hpp"
#include "debug.hpp"

namespace no {

ogg_vorbis_audio_source::ogg_vorbis_audio_source(const std::string& path) {
	file::read(path, file_stream);
	ov_callbacks callbacks = {
		// read
		[](void* pointer, size_t objectSize, size_t objectCount, void* source) -> size_t {
			size_t size = objectSize * objectCount;
			io_stream* file_stream = (io_stream*)source;
			size_t remaining = file_stream->size_left_to_read();
			if (size > remaining) {
				size = remaining;
			}
			file_stream->read((char*)pointer, size);
			return size / objectSize;
		},
		// seek
		[](void* source, ogg_int64_t offset, int whence) -> int {
			return -1;
		},
		// close
		[](void* source) -> int {
			return 0;
		},
		// tell
		[](void* source) -> long {
			return -1;
		}
	};

	MESSAGE("Loading ogg file: " << path);
	int result = ov_open_callbacks(&file_stream, &file, nullptr, 0, callbacks);
	if (result != 0) {
		WARNING("The stream is invalid. Error: " << result);
		return;
	}

	vorbis_info* info = ov_info(&file, -1);
	channels = info->channels;
	frequency = info->rate;
	INFO("Ogg file info:"
		 << "\nChannels: " << info->channels
		 << "\nFrequency: " << info->rate
	);

	char temp[4192];
	int bitStream = 0;
	while (true) {
		size_t bytes = ov_read(&file, temp, 4192, 0, 2, 1, &bitStream);
		if (bytes == 0) {
			break;
		}
		pcm_stream.write(temp, bytes);
	}
}

ogg_vorbis_audio_source::~ogg_vorbis_audio_source() {
	ov_clear(&file);
}

size_t ogg_vorbis_audio_source::size() const {
	return pcm_stream.size();
}

audio_data_format ogg_vorbis_audio_source::format() const {
	return audio_data_format::pulse_code_modulation;
}

const io_stream& ogg_vorbis_audio_source::stream() const {
	return pcm_stream;
}

}
