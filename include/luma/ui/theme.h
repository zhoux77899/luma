#pragma once

#include "luma/core/display.h"

namespace luma {
namespace theme {

constexpr Color kBootCanvas{0x08, 0x08, 0x08};
constexpr Color kCanvas{0x1C, 0x1C, 0x1C};
constexpr Color kPrimaryText{0xFF, 0xFF, 0xFB};
constexpr Color kSecondaryText{0x91, 0x98, 0x9F};
constexpr Color kAccent{0x7B, 0x90, 0xD2};
constexpr Color kTsuyukusa{0x2E, 0xA9, 0xDF};
constexpr Color kYamabuki{0xFF, 0xB1, 0x1B};
constexpr Color kWakatake{0x5D, 0xAC, 0x81};
constexpr Color kBenihi{0xF7, 0x5C, 0x2F};
constexpr Color kAraisyu{0xFB, 0x96, 0x6E};
constexpr Color kAomidori{0x00, 0xAA, 0x90};
constexpr Color kMizu{0x81, 0xC7, 0xD4};
constexpr Color kTsutsuji{0xE0, 0x3C, 0x8A};

inline Color appCardColor(int index) {
    constexpr Color kColors[] = {kTsuyukusa, kYamabuki, kWakatake, kBenihi,
                                 kAraisyu,   kAomidori, kMizu,     kTsutsuji};
    if (index < 0) {
        return kTsuyukusa;
    }
    return kColors[index % 8];
}

}  // namespace theme
}  // namespace luma

