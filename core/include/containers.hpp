#pragma once

#include "debug.hpp"

namespace no {

template<typename T>
class shifting_2d_array {
public:

	shifting_2d_array() = default;
	shifting_2d_array(const shifting_2d_array<T>&) = delete;
	shifting_2d_array(shifting_2d_array<T>&& that) : values(std::move(that.values)) {
		std::swap(offset, that.offset);
		std::swap(size, that.size);
	}

	shifting_2d_array<T>& operator=(const shifting_2d_array<T>&) = delete;
	shifting_2d_array<T>& operator=(shifting_2d_array<T>&& that) {
		std::swap(offset, that.offset);
		std::swap(size, that.size);
		std::swap(values, that.values);
	}

	void resize_and_reset(vector2i new_size, T value) {
		offset = 0;
		size = new_size;
		values.clear();
		values.insert(values.begin(), size.x * size.y, value);
	}

	void shift_left(const std::vector<T>& heights) {
		ASSERT((int)heights.size() == size.y);
		for (int x = size.x - 1; x > 0; x--) {
			for (int y = 0; y < size.y; y++) {
				int current = y * size.x + x;
				int before = current - 1;
				T height = values[current];
				values[current] = values[before];
				values[before] = height;
			}
		}
		for (int row = 0; row < size.y; row++) {
			values[row * size.x] = heights[row];
		}
		offset.x--;
	}

	void shift_right(const std::vector<T>& heights) {
		ASSERT((int)heights.size() == size.y);
		for (int x = 0; x < size.x - 1; x++) {
			for (int y = 0; y < size.y; y++) {
				int current = y * size.x + x;
				int before = current + 1;
				T height = values[current];
				values[current] = values[before];
				values[before] = height;
			}
		}
		for (int row = 0; row < size.y; row++) {
			values[row * size.x + size.x - 1] = heights[row];
		}
		offset.x++;
	}

	void shift_up(const std::vector<T>& heights) {
		ASSERT((int)heights.size() == size.x);
		for (int y = size.y - 1; y > 0; y--) {
			for (int x = 0; x < size.x; x++) {
				int current = y * size.x + x;
				int before = (y - 1) * size.x + x;
				T height = values[current];
				values[current] = values[before];
				values[before] = height;
			}
		}
		for (int column = 0; column < size.x; column++) {
			values[column] = heights[column];
		}
		offset.y--;
	}

	void shift_down(const std::vector<T>& heights) {
		ASSERT((int)heights.size() == size.x);
		for (int y = 0; y < size.y - 1; y++) {
			for (int x = 0; x < size.x; x++) {
				int current = y * size.x + x;
				int before = (y + 1) * size.x + x;
				T height = values[current];
				values[current] = values[before];
				values[before] = height;
			}
		}
		const int row_index = (size.y - 1) * size.x;
		for (int column = 0; column < size.x; column++) {
			values[row_index + column] = heights[column];
		}
		offset.y++;
	}

	void load(vector2i begin, vector2i end, const std::vector<T>& value) {
		size_t i = 0;
		for (int y = begin.y; y < end.y; y++) {
			for (int x = begin.x; x < end.x; x++) {
				values[local_index(x, y)] = value[i];
				i++;
			}
		}
	}

	void fill(vector2i begin, vector2i end, T value) {
		vector2i area = end - begin;
		std::vector<T> heights;
		heights.insert(heights.begin(), area.x * area.y, value);
		load(begin, end, heights);
	}

	int local_index(int x, int y) const {
		return (y - offset.y) * size.x + x - offset.x;
	}

	T at(int x, int y) const {
		return values[local_index(x, y)];
	}

	void set(int x, int y, T value) {
		values[local_index(x, y)] = value;
	}

	void add(int x, int y, T value) {
		values[local_index(x, y)] += value;
	}

	vector2i position() const {
		return offset;
	}

	int x() const {
		return offset.x;
	}

	int y() const {
		return offset.y;
	}

	int columns() const {
		return size.x;
	}

	int rows() const {
		return size.y;
	}

	int count() const {
		return size.x * size.y;
	}
	
	bool is_out_of_bounds(int x, int y) const {
		return x - offset.x < 0 || y - offset.y < 0 || x - offset.x >= size.x || y - offset.y >= size.y;
	}

	void write(io_stream& stream) const {
		stream.write(offset);
		stream.write(size);
		stream.write((uint32_t)values.size());
		for (auto& value : values) {
			stream.write(value);
		}
	}
	
	void read(io_stream& stream) {
		offset = stream.read<vector2i>();
		size = stream.read<vector2i>();
		uint32_t count = stream.read<uint32_t>();
		values.clear();
		values.reserve(count);
		for (uint32_t i = 0; i < count; i++) {
			values.push_back(stream.read<T>());
		}
	}

private:

	vector2i offset;
	vector2i size;
	std::vector<T> values;

};

}
