#pragma once

#include "surface.hpp"
#include "math.hpp"

#include <string>

namespace no {

class font {
public:

	struct text_size {
		vector2i size;
		int min_y = 0;
		int max_y = 0;
		int rows = 1;
	};

	font() = default;
	font(const std::string& path, int size);
	font(const font&) = delete;
	font(font&&) noexcept;
	~font();

	font& operator=(const font&) = delete;
	font& operator=(font&&);

	void render(surface& surface, const std::string& text, uint32_t color = 0x00FFFFFF) const;
	surface render(const std::string& text, uint32_t color = 0x00FFFFFF) const;
	text_size size(const std::string& text) const;

	static bool exists(const std::string& path);

private:

	class font_face;
	font_face* face = nullptr;

	std::pair<uint32_t*, vector2i> render_text(const std::string& text, uint32_t color) const;

	float line_space = 1.25f;

};

}
