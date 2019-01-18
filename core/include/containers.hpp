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

	void shift_left() {
		for (int x = size.x - 1; x > 0; x--) {
			for (int y = 0; y < size.y; y++) {
				int current = y * size.x + x;
				int before = current - 1;
				T value = values[current];
				values[current] = values[before];
				values[before] = value;
			}
		}
		offset.x--;
	}

	void shift_right() {
		for (int x = 0; x < size.x - 1; x++) {
			for (int y = 0; y < size.y; y++) {
				int current = y * size.x + x;
				int before = current + 1;
				T value = values[current];
				values[current] = values[before];
				values[before] = value;
			}
		}
		offset.x++;
	}

	void shift_up() {
		for (int y = size.y - 1; y > 0; y--) {
			for (int x = 0; x < size.x; x++) {
				int current = y * size.x + x;
				int before = (y - 1) * size.x + x;
				T value = values[current];
				values[current] = values[before];
				values[before] = value;
			}
		}
		offset.y--;
	}

	void shift_down() {
		for (int y = 0; y < size.y - 1; y++) {
			for (int x = 0; x < size.x; x++) {
				int current = y * size.x + x;
				int before = (y + 1) * size.x + x;
				T value = values[current];
				values[current] = values[before];
				values[before] = value;
			}
		}
		offset.y++;
	}

	void shift_left(const std::vector<T>& new_values) {
		ASSERT((int)heights.size() == size.y);
		shift_left();
		for (int row = 0; row < size.y; row++) {
			values[row * size.x] = new_values[row];
		}
	}

	void shift_right(const std::vector<T>& new_values) {
		ASSERT((int)heights.size() == size.y);
		shift_right();
		for (int row = 0; row < size.y; row++) {
			values[row * size.x + size.x - 1] = new_values[row];
		}
	}

	void shift_up(const std::vector<T>& new_values) {
		ASSERT((int)heights.size() == size.x);
		shift_up();
		for (int column = 0; column < size.x; column++) {
			values[column] = new_values[column];
		}
	}

	void shift_down(const std::vector<T>& new_values) {
		ASSERT((int)heights.size() == size.x);
		shift_down();
		const int row_index = (size.y - 1) * size.x;
		for (int column = 0; column < size.x; column++) {
			values[row_index + column] = new_values[column];
		}
	}

	void write(io_stream& stream, int stride, T filler) const {
		size_t old_write = stream.write_index();
		for (int x = 0; x < size.x; x++) {
			for (int y = 0; y < size.y; y++) {
				size_t write_index = ((offset.y + y) * stride + offset.x + x) * sizeof(T);
				size_t old_size = stream.size();
				stream.resize_if_needed(write_index + sizeof(T));
				size_t new_size = stream.size();
				if (new_size - old_size > 0) {
					stream.set_write_index(old_size);
					for (size_t i = old_size; i < new_size; i += sizeof(T)) {
						stream.write(filler);
					}
				}
				stream.set_write_index(write_index);
				stream.write(values[y * size.x + x]);
			}
		}
		stream.set_write_index(old_write);
	}

	void read(io_stream& stream, int stride, T filler) {
		size_t old_read = stream.read_index();
		for (int x = 0; x < size.x; x++) {
			for (int y = 0; y < size.y; y++) {
				size_t read_index = ((offset.y + y) * stride + offset.x + x) * sizeof(T);
				if (read_index + sizeof(T) > stream.size()) {
					values[y * size.x + x] = filler;
					continue;
				}
				stream.set_read_index(read_index);
				values[y * size.x + x] = stream.read<T>();
			}
		}
		stream.set_read_index(old_read);
	}

	void shift_left(io_stream& stream, int stride, T filler) {
		shift_left();
		for (int y = 0; y < size.y; y++) {
			size_t read_index = ((offset.y + y) * stride + offset.x) * sizeof(T);
			if (read_index + sizeof(T) > stream.size()) {
				values[y * size.x] = filler;
				continue;
			}
			stream.set_read_index(read_index);
			values[y * size.x] = stream.read<T>();
		}
	}

	void shift_right(io_stream& stream, int stride, T filler) {
		shift_right();
		for (int y = 0; y < size.y; y++) {
			size_t read_index = ((offset.y + y) * stride + offset.x + size.x - 1) * sizeof(T);
			if (read_index + sizeof(T) > stream.size()) {
				values[y * size.x + size.x - 1] = filler;
				continue;
			}
			stream.set_read_index(read_index);
			values[y * size.x + size.x - 1] = stream.read<T>();
		}
	}

	void shift_up(io_stream& stream, int stride, T filler) {
		shift_up();
		for (int x = 0; x < size.x; x++) {
			size_t read_index = (offset.y * stride + offset.x + x) * sizeof(T);
			if (read_index + sizeof(T) > stream.size()) {
				values[x] = filler;
				continue;
			}
			stream.set_read_index(read_index);
			values[x] = stream.read<T>();
		}
	}

	void shift_down(io_stream& stream, int stride, T filler) {
		shift_down();
		for (int x = 0; x < size.x; x++) {
			size_t read_index = ((size.y - 1 + offset.y) * stride + offset.x + x) * sizeof(T);
			if (read_index + sizeof(T) > stream.size()) {
				values[(size.y - 1) * size.x + x] = filler;
				continue;
			}
			stream.set_read_index(read_index);
			values[(size.y - 1) * size.x + x] = stream.read<T>();
		}
	}
	
private:

	vector2i offset;
	vector2i size;
	std::vector<T> values;

};

}