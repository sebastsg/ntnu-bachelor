#include "io.hpp"

#include <fstream>
#include <filesystem>

namespace no {

template<typename It>
static std::vector<std::string> iterate_entries_in_directory(const std::string& path, entry_inclusion inclusion) {
	std::vector<std::string> files;
	auto directory = It(path);
	for (auto& entry : directory) {
		if (entry.is_directory() && inclusion == entry_inclusion::only_files) {
			continue;
		}
		if (!entry.is_directory() && inclusion == entry_inclusion::only_directories) {
			continue;
		}
		files.push_back(entry.path().string());
	}
	return files;
}

std::vector<std::string> entries_in_directory(const std::string& path, entry_inclusion inclusion, bool recursive) {
	if (recursive) {
		return iterate_entries_in_directory<std::filesystem::recursive_directory_iterator>(path, inclusion);
	} else {
		return iterate_entries_in_directory<std::filesystem::directory_iterator>(path, inclusion);
	}
}

std::string file_extension_in_path(const std::string& path) {
	size_t last_dot_index = path.rfind('.');
	if (last_dot_index == std::string::npos) {
		return "";
	}
	return path.substr(last_dot_index);
}

io_stream::io_stream(size_t size) {
	allocate(size);
}

io_stream::io_stream(char* data, size_t size, construct_by construction) {
	switch (construction) {
	case construct_by::copy:
		write(data, size);
		write_position = end;
		break;
	case construct_by::move:
		begin = data;
		end = begin + size;
		read_position = begin;
		write_position = end;
		break;
	case construct_by::shallow_copy:
		begin = data;
		end = begin + size;
		read_position = begin;
		write_position = end;
		owner = false;
		break;
	}
}

io_stream::io_stream(io_stream&& that) {
	std::swap(begin, that.begin);
	std::swap(end, that.end);
	std::swap(read_position, that.read_position);
	std::swap(write_position, that.write_position);
	std::swap(owner, that.owner);
}

io_stream::~io_stream() {
	free();
}

io_stream& io_stream::operator=(io_stream&& that) {
	std::swap(begin, that.begin);
	std::swap(end, that.end);
	std::swap(read_position, that.read_position);
	std::swap(write_position, that.write_position);
	std::swap(owner, that.owner);
	return *this;
}

void io_stream::allocate(size_t size) {
	if (begin) {
		resize(size);
		return;
	}
	begin = new char[size];
	end = begin;
	if (begin) {
		end += size;
	}
	read_position = begin;
	write_position = begin;
}

void io_stream::resize(size_t new_size) {
	if (!begin) {
		allocate(new_size);
		return;
	}
	size_t old_read_index = read_index();
	size_t old_write_index = write_index();
	char* old_begin = begin;
	size_t old_size = size();
	size_t copy_size = std::min(old_size, new_size);
	begin = new char[new_size];
	memcpy(begin, old_begin, copy_size);
	delete[] old_begin;
	end = begin;
	if (begin) {
		end += new_size;
	}
	read_position = begin + old_read_index;
	write_position = begin + old_write_index;
}

void io_stream::resize_if_needed(size_t size_to_write) {
	if (size_to_write > size_left_to_write()) {
		// size() might be 0, so make sure the data fits comfortably.
		size_t new_size = size() * 2 + size_to_write + 64;
		resize(new_size);
	}
}

void io_stream::free() {
	if (owner) {
		delete[] begin;
	}
	begin = nullptr;
	end = nullptr;
	read_position = nullptr;
	write_position = nullptr;
}

void io_stream::shift_read_to_begin() {
	size_t shift_size = write_position - read_position;
	memcpy(begin, read_position, shift_size); // copy read-to-write to begin-to-size.
	read_position = begin;
	write_position = begin + shift_size;
}

void io_stream::set_read_index(size_t index) {
	if (index >= size()) {
		read_position = end;
	} else {
		read_position = begin + index;
	}
}

void io_stream::set_write_index(size_t index) {
	if (index >= size()) {
		write_position = end;
	} else {
		write_position = begin + index;
	}
}

void io_stream::move_read_index(long long size) {
	long long index = (long long)read_index() + size;
	set_read_index((size_t)index);
}

void io_stream::move_write_index(long long size) {
	long long index = (long long)write_index() + size;
	set_write_index((size_t)index);
}

size_t io_stream::size() const {
	return (size_t)(end - begin);
}

size_t io_stream::size_left_to_write() const {
	return (size_t)(end - write_position);
}

size_t io_stream::size_left_to_read() const {
	return (size_t)(write_position - read_position);
}

size_t io_stream::read_index() const {
	return (size_t)(read_position - begin);
}

size_t io_stream::write_index() const {
	return (size_t)(write_position - begin);
}

char* io_stream::at(size_t index) const {
	return begin + index;
}

char* io_stream::at_read() const {
	return read_position;
}

char* io_stream::at_write() const {
	return write_position;
}

size_t io_stream::read_line(char* destination, size_t max_size, bool remove_newline) {
	if (size_left_to_read() < 2) {
		return 0;
	}
	size_t i = 0;
	while (read_position[i] != '\n') {
		destination[i] = read_position[i];
		if (destination[i] == '\0') {
			read_position += i;
			return i;
		}
		++i;
		if (i >= max_size) {
			read_position += i;
			destination[max_size - 1] = '\0';
			return max_size - 1;
		}
	}
	size_t end_of_the_line = i + 1;
	if (remove_newline) {
		--i; // remove last increment
		if (i - 1 > 0 && read_position[i - 2] == '\r') {
			--i; // remove \r
		}
	} else {
		destination[i] = '\n';
	}
	memcpy(destination, read_position, i);
	read_position += end_of_the_line;
	destination[i] = '\0';
	return i;
}

std::string io_stream::read_line(bool remove_newline) {
	std::string result;
	char buffer[256];
	while (true) {
		size_t count = read_line(buffer, 256, remove_newline);
		if (count == 0) {
			break;
		}
		result += buffer;
	}
	return result;
}

int io_stream::find_first(const std::string& key, size_t start) const {
	const char* search_start = begin + start;
	char* found = (char*)strstr(search_start, key.c_str());
	if (!found) {
		return -1;
	}
	found -= (size_t)begin;
	return (int)found;
}

int io_stream::find_last(const std::string& key, size_t start) const {
	char* found = begin + start;
	while (found) {
		char* previous = found;
		found = strstr(found, key.c_str());
		if (!found) {
			previous -= (size_t)begin;
			return (int)previous;
		}
		++found;
	}
	return -1;
}

char* io_stream::data() const {
	return begin;
}

bool io_stream::is_owner() const {
	return owner;
}

namespace file {

void write(const std::string& path, const std::string& source) {
	std::filesystem::create_directories(std::filesystem::path(path).parent_path());
	std::ofstream file(path, std::ios::binary);
	if (file.is_open()) {
		file << source;
	}
}

void write(const std::string& path, const char* source, size_t size) {
	std::filesystem::create_directories(std::filesystem::path(path).parent_path());
	std::ofstream file(path, std::ios::binary);
	if (file.is_open()) {
		file.write(source, size);
	}
}

void write(const std::string& path, io_stream& source) {
	write(path, source.data(), source.write_index());
}

void append(const std::string& path, const std::string& source) {
	std::ofstream file(path, std::ios::app);
	if (file.is_open()) {
		file << source;
	}
}

std::string read(const std::string& path) {
	std::ifstream file(path, std::ios::binary);
	if (!file.is_open()) {
		return "";
	}
	std::stringstream result;
	result << file.rdbuf();
	return result.str();
}

void read(const std::string& path, io_stream& stream) {
	std::ifstream file(path, std::ios::binary);
	if (file.is_open()) {
		std::stringstream result;
		result << file.rdbuf();
		stream.write(result.str().c_str(), result.str().size());
	}
}

}

}
