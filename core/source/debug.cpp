#include "debug.hpp"
#include "io.hpp"
#include "assets.hpp"
#include "loop.hpp"

#include <chrono>

namespace no {

namespace debug {

static const size_t LOG_COUNT = 4;

static struct {
	std::string path;
	std::string temp_buffer;
	std::string final_buffer;
} logs[LOG_COUNT];

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
	logs[index].final_buffer += "<td colspan=\"" + std::to_string(col_span) + "\">" + message + "</td>";
}

static void flush(int index, message_type type, const char* file, const char* func, int line) {
	if (logs[index].temp_buffer.size() == 0) {
		return;
	}
	auto time_since_epoch = std::chrono::system_clock::now().time_since_epoch();
	auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(time_since_epoch).count();
	std::string ms_in_second = std::to_string(milliseconds % 1000);

	const std::string type_string[] = { "message", "warning", "critical", "info" };
	logs[index].final_buffer += "\r\n<tr class=\"" + type_string[(size_t)type] + "\">";
	write_field(index, "<b>" + current_local_time_string() + "</b>." + ms_in_second);
	write_field(index, logs[index].temp_buffer);
	write_field(index, file);
	write_field(index, get_html_compatible_string(func));
	write_field(index, std::to_string(line));
	logs[index].final_buffer += "</tr>";
	file::append(logs[index].path, logs[index].final_buffer);
	logs[index].final_buffer = "";
	logs[index].temp_buffer = "";
}

static void initialize() {
	if (is_initialized) {
		return;
	}
	is_initialized = true;
	template_buffer = file::read(asset_path("debug/template.html"));
	for (int i = 0; i < LOG_COUNT; i++) {
		logs[i].path = "_Debug_Log_" + std::to_string(i) + ".html";
		file::write(logs[i].path, template_buffer);
	}
}

void append(int index, message_type type, const char* file, const char* func, int line, const std::string& message) {
	ASSERT(index >= 0 && index < LOG_COUNT);
	initialize();
	logs[index].temp_buffer = get_html_compatible_string(message);
	flush(index, type, file, func, line);
}

}

}
