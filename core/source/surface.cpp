#include "surface.hpp"
#include "debug.hpp"
#include "png.hpp"

#include <cstring>

namespace no {

surface::surface(surface&& that) {
	pixels = that.pixels;
	size = that.size;
	format_ = that.format_;
	that.pixels = nullptr;
	that.size = 0;
}

surface::surface(const std::string& path) {
	surface result = load_png(path);
	pixels = result.pixels;
	size = result.size;
	format_ = result.format_;
	result.pixels = nullptr;
}

surface::surface(uint32_t* source_pixels, int width, int height, pixel_format format, construct_by construction) : size(width, height), format_(format) {
	ASSERT(source_pixels);
	switch (construction) {
	case construct_by::copy:
		pixels = new uint32_t[width * height];
		memcpy(pixels, source_pixels, width * height * sizeof(uint32_t));
		break;
	case construct_by::move:
		pixels = source_pixels;
		break;
	default:
		break;
	}
}

surface::surface(int width, int height, pixel_format format) : size(width, height), format_(format) {
	pixels = new uint32_t[width * height];
}

surface::surface(int width, int height, pixel_format format, uint32_t color) : surface(width, height, format) {
	std::fill_n(pixels, width * height, color);
}

surface::~surface() {
	delete[] pixels;
}

void surface::resize(int width, int height) {
	if (size.x >= width && size.y >= height) {
		size = { width, height };
		return;
	}
	uint32_t* old_pixels = pixels;
	int count = width * height;
	pixels = new uint32_t[count];
	for (int y = 0; y < height; y++) {
		memcpy(pixels + y * width, old_pixels + y * size.x, size.x * sizeof(uint32_t));
		memset(pixels + y * width + size.x, 0x00, width - size.x * sizeof(uint32_t));
	}
	size = { width, height };
	delete[] old_pixels;
}

void surface::flip_frames_horizontally(int frames) {
	const int frame_width = size.x / frames;
	const int frame_half_width = frame_width / 2 - frame_width % 2;
	int frame_begin_x = 0;
	for (int frame = 0; frame < frames; frame++) {
		const int frame_middle_x = frame_begin_x + frame_half_width;
		for (int x = frame_begin_x; x < frame_middle_x; x++) {
			for (int y = 0; y < size.y; y++) {
				const int row = y * size.x;
				const int right_column = frame_begin_x + frame_width - (x - frame_begin_x) - 1;
				uint32_t left = pixels[row + x];
				pixels[row + x] = pixels[row + right_column];
				pixels[row + right_column] = left;
			}
		}
		frame_begin_x += frame_width;
	}
}

void surface::flip_horizontally() {
	const int half_width = size.x / 2 - size.x % 2;
	for (int x = 0; x < half_width; x++) {
		for (int y = 0; y < size.y; y++) {
			const int row = y * size.x;
			uint32_t left = pixels[row + x];
			pixels[row + x] = pixels[row + size.x - x - 1];
			pixels[row + size.x - x - 1] = left;
		}
	}
}

void surface::flip_vertically() {
	const int half_height = size.y / 2;
	uint32_t* temporary_rows = new uint32_t[size.x * sizeof(uint32_t)];
	for (int y = 0; y < half_height; y++) {
		memcpy(temporary_rows, pixels + y * size.x, size.x * sizeof(uint32_t));
		memcpy(pixels + y * size.x, pixels + (size.y - y - 1) * size.x, size.x * sizeof(uint32_t));
		memcpy(pixels + (size.y - y - 1) * size.x, temporary_rows, size.x * sizeof(uint32_t));
	}
	delete[] temporary_rows;
}

uint32_t* surface::data() const {
	return pixels;
}

uint32_t surface::at(int x, int y) const {
	return pixels[y * size.x + x];
}

uint32_t surface::at(int index) const {
	return pixels[index];
}

void surface::clear(uint32_t color) {
	std::fill_n(pixels, size.x * size.y, color);
}

void surface::render(uint32_t* source_pixels, int width, int height) {
	if (width > size.x || height > size.y) {
		resize(width, height);
	}
	if (size.x == width && size.y == height) {
		memcpy(pixels, source_pixels, width * height * sizeof(uint32_t));
		return;
	}
	for (int y = 0; y < height; y++) {
		memcpy(pixels + y * size.x, source_pixels + y * width, width * sizeof(uint32_t));
	}
}

void surface::render_horizontal_line(uint32_t color, int x, int y, int width) {
	std::fill_n(pixels + y * size.x + x, width, color);
}

void surface::render_vertical_line(uint32_t color, int x, int y, int height) {
	const int y1 = y * size.x + x;
	const int y2 = (y + height) * size.x + x;
	for (int i = y1; i < y2; i += size.x) {
		pixels[i] = color;
	}
}

void surface::render_rectangle(uint32_t color, int x, int y, int width, int height) {
	const int y1 = y * size.x + x;
	const int y2 = (y + height) * size.x + x;
	for (int i = y1; i < y2; i += size.x) {
		std::fill_n(pixels + i, width, color);
	}
}

void surface::render_circle(uint32_t color, int center_x, int center_y, int radius) {
	ASSERT(center_x - radius >= 0);
	ASSERT(center_y - radius >= 0);
	ASSERT(center_x + radius < size.x);
	ASSERT(center_y + radius < size.y);
	const int diameter = radius * 2;
	int x = radius - 1;
	int y = 0;
	int dx = 1;
	int dy = 1;
	int err = dx - diameter;
	while (x >= y) {
		std::fill_n(pixels + (center_y + y) * size.x + center_x - x, x * 2, color);
		std::fill_n(pixels + (center_y + x) * size.x + center_x - y, y * 2, color);
		std::fill_n(pixels + (center_y - y) * size.x + center_x - x, x * 2, color);
		std::fill_n(pixels + (center_y - x) * size.x + center_x - y, y * 2, color);
		if (err <= 0) {
			y++;
			err += dy;
			dy += 2;
		}
		if (err > 0) {
			x--;
			dx += 2;
			err += dx - diameter;
		}
	}
}

vector2i surface::dimensions() const {
	return size;
}

int surface::width() const {
	return size.x;
}

int surface::height() const {
	return size.y;
}

int surface::count() const {
	return size.x * size.y;
}

pixel_format surface::format() const {
	return format_;
}

}
