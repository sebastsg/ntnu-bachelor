#include "debug.hpp"

#if ENABLE_DEBUG

#include "io.hpp"
#include "assets.hpp"
#include "loop.hpp"

#include <chrono>
#include <mutex>
#include <iostream>
#include <iomanip>

std::ostream& operator<<(std::ostream& out, no::debug::message_type message) {
	switch (message) {
	case no::debug::message_type::message: return out << "message";
	case no::debug::message_type::warning: return out << "warning";
	case no::debug::message_type::critical: return out << "critical";
	case no::debug::message_type::info: return out << "info";
	default: return out << "";
	}
}

namespace no::debug {

static std::mutex log_mutex;

std::string current_time_ms_string() {
	auto time_since_epoch = std::chrono::system_clock::now().time_since_epoch();
	auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(time_since_epoch).count();
	return std::to_string(milliseconds % 1000);
}

#if ENABLE_HTML_LOG

static struct {
	std::string path;
	std::string temp_buffer;
	std::string final_buffer;
} html_logs[HTML_LOG_COUNT];

static std::string template_buffer;
static bool is_initialized = false;

static void replace_substring(std::string& string, const std::string& substring, const std::string& replace_with) {
	auto pos = string.find(substring);
	while (pos != std::string::npos) {
		string.replace(pos, substring.size(), replace_with);
		pos = string.find(substring, pos + replace_with.size());
	}
}

static std::string get_html_compatible_string(std::string string) {
	replace_substring(string, "&", "&amp;");
	replace_substring(string, ">", "&gt;");
	replace_substring(string, "<", "&lt;");
	replace_substring(string, "\n", "<br>");
	replace_substring(string, "[b]", "<b>");
	replace_substring(string, "[/b]", "</b>");
	return string;
}

static void write_field(int index, const std::string& message, int col_span = 1) {
	html_logs[index].final_buffer += "<td colspan=\"" + std::to_string(col_span) + "\">" + message + "</td>";
}

static void flush(int index, message_type type, const char* file, const char* func, int line) {
	if (html_logs[index].temp_buffer.size() == 0) {
		return;
	}
	html_logs[index].final_buffer += "\r\n<tr class=\"" + STRING(type) + "\">";
	write_field(index, "<b>" + current_local_time_string() + "</b>." + current_time_ms_string());
	write_field(index, html_logs[index].temp_buffer);
	write_field(index, file);
	write_field(index, get_html_compatible_string(func));
	write_field(index, std::to_string(line));
	html_logs[index].final_buffer += "</tr>";
	file::append(html_logs[index].path, html_logs[index].final_buffer);
	html_logs[index].final_buffer = "";
	html_logs[index].temp_buffer = "";
}

static void initialize() {
	if (is_initialized) {
		return;
	}
	is_initialized = true;
	template_buffer = file::read(asset_path("debug/template.html"));
	for (int i = 0; i < HTML_LOG_COUNT; i++) {
		html_logs[i].path = "_Debug_Log_" + std::to_string(i) + ".html";
		file::write(html_logs[i].path, template_buffer);
	}
}

#endif

void append(int index, message_type type, const char* file, const char* func, int line, const std::string& message) {
	std::lock_guard lock{ log_mutex };
#if ENABLE_HTML_LOG
	ASSERT(index >= 0 && index < HTML_LOG_COUNT);
	initialize();
	html_logs[index].temp_buffer = get_html_compatible_string(message);
	flush(index, type, file, func, line);
#endif
#if ENABLE_STDOUT_LOG
	std::cout << std::left << std::setw(13) << (current_local_time_string() + "." + current_time_ms_string()) << std::setw(1);
	std::cout << std::internal << type << ": " << message << "\n";
#endif
}

}

#endif
