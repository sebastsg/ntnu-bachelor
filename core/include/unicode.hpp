#pragma once

#include <string>
#include <cstdint>

namespace no {

namespace unicode {

const uint32_t replacement_character = 0xFFFD;
const uint32_t byte_order_mark = 0xFEFF;
const uint32_t byte_order_mark_swapped = 0xFFFE;

}

namespace utf8 {

std::string from_latin1(const std::string& latin1_string);
uint32_t next_character(const std::string& utf8_string, size_t* index);

}

}
