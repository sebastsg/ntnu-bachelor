#pragma once

#include <sstream>
#include <vector>

#define STRING(X) ((std::ostringstream&)(std::ostringstream{} << X)).str()
#define CSTRING(X) STRING(X).c_str()

namespace no {

enum class entry_inclusion { everything, only_files, only_directories };

std::vector<std::string> entries_in_directory(const std::string& path, entry_inclusion inclusion);
std::string file_extension_in_path(const std::string& path);

class io_stream {
public:

	enum class construct_by { copy, move, shallow_copy };

	io_stream();
	io_stream(size_t size);
	io_stream(char* data, size_t size, construct_by construction);
	io_stream(const io_stream&) = delete;
	io_stream(io_stream&&);
	~io_stream();

	io_stream& operator=(const io_stream&) = delete;
	io_stream& operator=(io_stream&&);

	void allocate(size_t size);
	void resize(size_t new_size);
	void resize_if_needed(size_t size_to_write);
	void free();

	// move everything from the read position to the beginning
	// useful when we have extracted some data, and want to 
	// read the rest with a read index of 0
	void shift_read_to_begin();

	void set_read_index(size_t index);
	void set_write_index(size_t index);

	void move_read_index(long long size);
	void move_write_index(long long size);

	size_t size() const;
	size_t size_left_to_write() const;
	size_t size_left_to_read() const;
	size_t read_index() const;
	size_t write_index() const;

	char* at(size_t index) const;
	char* at_read() const;
	char* at_write() const;

	template<typename T>
	T read(size_t index) const {
		if (begin + index > end) {
			return {};
		}
		T value;
		memcpy(&value, begin + index, sizeof(T));
		return value;
	}

	template<typename T>
	T read() {
		if (read_position + sizeof(T) > end) {
			return {};
		}
		T value;
		memcpy(&value, read_position, sizeof(T));
		read_position += sizeof(T);
		return value;
	}

	template<>
	std::string read() {
		size_t length = read<uint32_t>();
		if (length == 0) {
			return "";
		}
		if (read_position + length > end) {
			return {};
		}
		std::string result;
		result.insert(result.begin(), length, '\0');
		memcpy(&result[0], read_position, length);
		read_position += length;
		return result;
	}

	void read(char* destination, size_t size) {
		if (read_position + size > end) {
			return;
		}
		memcpy(destination, read_position, size);
		read_position += size;
	}

	template<typename T>
	void write(T value) {
		resize_if_needed(sizeof(T));
		memcpy(write_position, &value, sizeof(T));
		write_position += sizeof(T);
	}

	template<>
	void write(std::string value) {
		size_t size = value.size();
		write<uint32_t>(size);
		if (size == 0) {
			return;
		}
		resize_if_needed(size);
		memcpy(write_position, value.c_str(), size);
		write_position += size;
	}

	void write(const char* source, size_t size) {
		resize_if_needed(size);
		memcpy(write_position, source, size);
		write_position += size;
	}

	size_t read_line(char* destination, size_t max_size, bool remove_newline);
	std::string read_line(bool remove_newline);

	int find_first(const std::string& key, size_t start) const;
	int find_last(const std::string& key, size_t start) const;

	char* data() const;
	bool is_owner() const;

private:

	char* begin = nullptr;
	char* end = nullptr;
	char* read_position = nullptr;
	char* write_position = nullptr;
	bool owner = true;

};

namespace file {

void write(const std::string& path, const std::string& source);
void write(const std::string& path, const char* source, size_t size);
void write(const std::string& path, io_stream& source);
void append(const std::string& path, const std::string& source);
std::string read(const std::string& path);
void read(const std::string& path, io_stream& destination);

}

}
