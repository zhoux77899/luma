#pragma once

#include <cstdint>

namespace luma {
namespace assets {

struct RgbaMark {
    int width;
    int height;
    const uint8_t* rgba;
};

const RgbaMark* brandMark(int index);

}  // namespace assets
}  // namespace luma
