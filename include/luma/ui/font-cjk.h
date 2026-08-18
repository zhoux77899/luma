#pragma once

#include <cstdint>

namespace luma {
namespace font {

constexpr int kCjkGlyphWidth = 10;

const uint16_t* cjkGlyphRows(uint32_t code);
int cjkGlyphCount();

}  // namespace font
}  // namespace luma
