#pragma once

#include "luma/core/battery-types.h"
#include "luma/core/display.h"

namespace luma {
namespace theme {

constexpr Color kKuro{0x08, 0x08, 0x08};
constexpr Color kSumi{0x1C, 0x1C, 0x1C};
constexpr Color kGofun{0xFF, 0xFF, 0xFB};
constexpr Color kShironezumi{0xBD, 0xC0, 0xBA};
constexpr Color kGinnezumi{0x91, 0x98, 0x9F};
constexpr Color kBenimidori{0x7B, 0x90, 0xD2};

constexpr Color kBootCanvas = kKuro;
constexpr Color kCanvas = kKuro;
constexpr Color kPrimaryText = kGofun;
constexpr Color kSecondaryText = kGinnezumi;
constexpr Color kAccent = kBenimidori;
constexpr Color kTsuyukusa{0x2E, 0xA9, 0xDF};
constexpr Color kYamabuki{0xFF, 0xB1, 0x1B};
constexpr Color kWakatake{0x5D, 0xAC, 0x81};
constexpr Color kBenihi{0xF7, 0x5C, 0x2F};
constexpr Color kAraisyu{0xFB, 0x96, 0x6E};
constexpr Color kAomidori{0x00, 0xAA, 0x90};
constexpr Color kMizu{0x81, 0xC7, 0xD4};
constexpr Color kTsutsuji{0xE0, 0x3C, 0x8A};

struct Palette {
    Color boot_canvas;
    Color canvas;
    Color primary_text;
    Color secondary_text;
    Color accent;
    Color card;
};

inline Color batteryBandColor(BatteryBand band) {
    if (band == BatteryBand::Critical) {
        return kBenihi;
    }
    if (band == BatteryBand::Warn) {
        return kYamabuki;
    }
    return kWakatake;
}

inline Palette paletteFor(uint8_t theme_pref, Color accent = kAccent) {
    if (theme_pref == 1) {
        return Palette{kGofun, kGofun, kSumi, kGinnezumi, accent, kShironezumi};
    }
    return Palette{kKuro, kKuro, kGofun, kGinnezumi, accent, kSumi};
}

}  // namespace theme
}  // namespace luma
