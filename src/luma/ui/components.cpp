#include "luma/ui/components.h"

#include "luma/assets/battery-icons.h"
#include "luma/assets/wifi-icons.h"
#include "luma/ui/font.h"
#include "luma/ui/layout.h"

#include <cstdio>
#include <cstring>

namespace luma {
namespace {

int headerTextY(int size = 1) {
    const int height = font::kGlyphHeight * (size < 1 ? 1 : size);
    return (layout::kHeaderHeight - height) / 2;
}

int centeredY(int box_y, int box_h) { return box_y + (box_h - font::kGlyphHeight) / 2; }

}  // namespace

void formatCivilTime(const CivilTime& time, char* out, unsigned out_size) {
    if (out == nullptr || out_size == 0) {
        return;
    }
    if (!time.valid) {
        std::snprintf(out, out_size, "--:--");
        return;
    }
    std::snprintf(out, out_size, "%02u:%02u", static_cast<unsigned>(time.hour),
                  static_cast<unsigned>(time.minute));
}

void drawNetworkGlyph(DisplaySurface& display, const theme::Palette& palette, Point origin,
                      NetworkState state, SignalStrength strength) {
    if (state == NetworkState::Connected) {
        drawWifiListGlyph(display, palette, origin, strength);
        return;
    }
    display.drawMonoBitmap(origin, assets::kWifiListIconSize, assets::kWifiListIconSize,
                           assets::kWifiDisconnected, palette.primary_text);
}

void drawWifiListGlyph(DisplaySurface& display, const theme::Palette& palette, Point origin,
                       SignalStrength strength) {
    const Color on = palette.primary_text;
    const Color off = palette.secondary_text;
    const Color origin_color = strength == SignalStrength::None || strength == SignalStrength::Weakest
                                   ? off
                                   : on;
    const Color mid = (strength == SignalStrength::Strong || strength == SignalStrength::Mid) ? on
                                                                                              : off;
    const Color outer = strength == SignalStrength::Strong ? on : off;
    display.drawMonoBitmap(origin, assets::kWifiListIconSize, assets::kWifiListIconSize,
                           assets::kWifiListArc3, outer);
    display.drawMonoBitmap(origin, assets::kWifiListIconSize, assets::kWifiListIconSize,
                           assets::kWifiListArc2, mid);
    display.drawMonoBitmap(origin, assets::kWifiListIconSize, assets::kWifiListIconSize,
                           assets::kWifiListDot, origin_color);
}

void drawLockGlyph(DisplaySurface& display, const theme::Palette& palette, Point origin,
                   bool locked) {
    display.drawMonoBitmap(origin, assets::kWifiListIconSize, assets::kWifiListIconSize,
                           locked ? assets::kLockClosed : assets::kLockOpen, palette.primary_text);
}

void drawBatteryGlyph(DisplaySurface& display, const theme::Palette& palette, Point origin,
                      const BatteryReading& reading) {
    Color color = palette.secondary_text;
    if (reading.percent_valid) {
        color = theme::batteryBandColor(batteryBand(reading.percent));
    }
    display.drawMonoBitmap(origin, assets::kBatteryIconSize, assets::kBatteryIconSize,
                           assets::kBatteryOutline, color);
    const uint8_t fill = batteryFillLevel(reading);
    const uint16_t* layers[] = {nullptr, assets::kBatteryFill1, assets::kBatteryFill2,
                                assets::kBatteryFill3, assets::kBatteryFill4, assets::kBatteryFill5};
    for (uint8_t i = 1; i <= fill && i <= 5; ++i) {
        display.drawMonoBitmap(origin, assets::kBatteryIconSize, assets::kBatteryIconSize, layers[i],
                               color);
    }
}

void drawBatteryHistory(DisplaySurface& display, const theme::Palette& palette, Rect bounds,
                        const BatterySample* samples, int count) {
    if (samples == nullptr || count <= 0 || bounds.w <= 0 || bounds.h <= 0) {
        return;
    }
    constexpr int kSlots = 60;
    constexpr int kBarWidth = 1;
    constexpr int kBarGap = 1;
    constexpr int kStride = kBarWidth + kBarGap;
    const int chart_w = kSlots * kStride;
    int origin_x = bounds.x;
    if (bounds.w > chart_w) {
        origin_x += (bounds.w - chart_w) / 2;
    }
    int slot = 0;
    for (int i = 0; i < count && slot < kSlots; ++i) {
        if (i > 0 && batteryHistoryGap(samples[i - 1], samples[i]) && slot < kSlots) {
            ++slot;
        }
        if (slot >= kSlots) {
            break;
        }
        const BatteryReading& reading = samples[i].reading;
        if (reading.percent_valid) {
            int height = (bounds.h * static_cast<int>(reading.percent)) / 100;
            if (height < 1) {
                height = 1;
            }
            if (height > bounds.h) {
                height = bounds.h;
            }
            const Color color = theme::batteryBandColor(batteryBand(reading.percent));
            display.fillRect(
                {origin_x + slot * kStride, bounds.y + bounds.h - height, kBarWidth, height}, color);
        }
        ++slot;
    }
}

void ellipsizeToWidth(const char* src, char* dst, size_t dst_size, int max_width) {
    if (dst == nullptr || dst_size == 0) {
        return;
    }
    dst[0] = '\0';
    if (src == nullptr || max_width <= 0) {
        return;
    }
    if (font::textWidth(src, 1) <= max_width) {
        std::snprintf(dst, dst_size, "%s", src);
        return;
    }
    const int dots_w = font::textWidth("...", 1);
    const int budget = max_width - dots_w;
    if (budget <= 0) {
        std::snprintf(dst, dst_size, "...");
        return;
    }
    const char* cursor = src;
    const char* keep = src;
    uint32_t code = 0;
    int width = 0;
    while (font::nextCodepoint(cursor, code)) {
        const int glyph_w = font::glyphFor(code, false).width;
        if (width + glyph_w > budget) {
            break;
        }
        width += glyph_w;
        keep = cursor;
    }
    size_t n = static_cast<size_t>(keep - src);
    if (n + 4 > dst_size) {
        n = dst_size > 4 ? dst_size - 4 : 0;
    }
    if (n > 0) {
        std::memcpy(dst, src, n);
    }
    dst[n] = '\0';
    std::snprintf(dst + n, dst_size - n, "...");
}

void drawAppHeader(DisplaySurface& display, const theme::Palette& palette, const uint16_t* logo,
                   const char* title, const char* time, NetworkState state,
                   SignalStrength strength, const BatteryReading& battery) {
    display.fillRect(layout::kHeader, palette.canvas);
    if (logo != nullptr) {
        display.drawBitmap({layout::kHeaderLogoX, layout::kHeaderLogoY}, layout::kHeaderLogoSize,
                           layout::kHeaderLogoSize, logo);
    }
    const int title_x = layout::kHeaderLogoX + layout::kHeaderLogoSize + layout::kChromeInset;
    display.drawText({title_x, headerTextY(layout::kHeaderTitleSize)},
                     {palette.primary_text, layout::kHeaderTitleSize},
                     title != nullptr ? title : "");
    const char* label = time != nullptr ? time : "--:--";
    const int cluster_h =
        layout::kHeaderNetworkIconSize + layout::kHeaderStatusGap + font::kGlyphHeight;
    const int cluster_y = (layout::kHeaderHeight - cluster_h) / 2;
    const int time_x = layout::kWidth - layout::kChromeInset - font::textWidth(label, 1);
    drawNetworkGlyph(display, palette, {layout::kHeaderNetworkIconX, cluster_y}, state, strength);
    drawBatteryGlyph(display, palette, {layout::kHeaderBatteryIconX, cluster_y}, battery);
    display.drawText({time_x, cluster_y + layout::kHeaderNetworkIconSize + layout::kHeaderStatusGap},
                     {palette.primary_text, 1}, label);
}

void drawMenuItem(DisplaySurface& display, const theme::Palette& palette, Rect bounds,
                  const char* label, bool selected, const char* value, bool focused) {
    display.fillRect(bounds, palette.canvas);
    if (selected && focused) {
        display.drawRect(bounds, palette.accent);
    }
    display.drawText({bounds.x + 4, centeredY(bounds.y, bounds.h)},
                     {selected ? palette.primary_text : palette.secondary_text, 1},
                     label != nullptr ? label : "");
    if (value != nullptr && value[0] != '\0') {
        const int value_x = bounds.x + bounds.w - 4 - font::textWidth(value, 1);
        display.drawText({value_x, centeredY(bounds.y, bounds.h)}, {palette.primary_text, 1}, value);
    }
}

void drawProgressBar(DisplaySurface& display, const theme::Palette& palette, Rect bounds,
                     uint8_t percent) {
    display.fillRect(bounds, palette.secondary_text);
    if (percent == 0 || bounds.w <= 0) {
        return;
    }
    const unsigned clamped = percent > 100 ? 100 : percent;
    const int fill_w = static_cast<int>((static_cast<unsigned>(bounds.w) * clamped) / 100);
    if (fill_w > 0) {
        display.fillRect({bounds.x, bounds.y, fill_w, bounds.h}, palette.accent);
    }
}

void drawList(DisplaySurface& display, const theme::Palette& palette, const char* const* items,
              int count, int selected) {
    if (items == nullptr) {
        return;
    }
    for (int i = 0; i < count; ++i) {
        const Rect row{layout::kChromeInset, layout::kContentBoth.y + 4 + i * 16,
                       layout::kWidth - 2 * layout::kChromeInset, 14};
        drawMenuItem(display, palette, row, items[i], i == selected);
    }
}

void drawDialog(DisplaySurface& display, const theme::Palette& palette, const char* title,
                const char* body) {
    const Rect box{30, 30, 180, 75};
    display.fillRect(box, palette.canvas);
    display.drawRect(box, palette.accent);
    display.drawText({40, 40}, {palette.primary_text, 1}, title != nullptr ? title : "");
    display.drawText({40, 56}, {palette.secondary_text, 1}, body != nullptr ? body : "");
}

void drawFooterHints(DisplaySurface& display, const theme::Palette& palette, const KeyHint* hints,
                     int count) {
    display.fillRect(layout::kFooter, palette.canvas);
    if (hints == nullptr || count <= 0) {
        return;
    }

    constexpr int kChipPadX = 2;
    constexpr int kChipRadius = 2;
    constexpr int kChipToLabel = 3;
    constexpr int kGroupGap = 8;
    constexpr int kChipHeight = 13;

    int x = layout::kChromeInset;
    const int chip_y = layout::kFooter.y + (layout::kFooterHeight - kChipHeight) / 2;
    const int text_y = centeredY(layout::kFooter.y, layout::kFooter.h);

    for (int i = 0; i < count; ++i) {
        const char* key = hints[i].key;
        const char* label = hints[i].label != nullptr ? hints[i].label : "";
        if (key != nullptr && key[0] != '\0') {
            const int key_w = font::textWidth(key, 1);
            const Rect chip{x, chip_y, key_w + 2 * kChipPadX, kChipHeight};
            display.fillRoundRect(chip, kChipRadius, theme::kAomidori);
            display.drawText({x + kChipPadX, text_y}, {palette.primary_text, 1}, key);
            x += chip.w + kChipToLabel;
        }
        if (label[0] != '\0') {
            display.drawText({x, text_y}, {palette.secondary_text, 1}, label);
            x += font::textWidth(label, 1);
        }
        x += kGroupGap;
    }
}

void drawOverflowScrollbar(DisplaySurface& display, const theme::Palette& palette, Rect bounds,
                           int count, int start, int visible) {
    if (count <= visible || bounds.w <= 0 || bounds.h <= 0) {
        return;
    }
    display.fillRect(bounds, palette.secondary_text);
    int thumb_h = (visible * bounds.h) / count;
    if (thumb_h < 4) {
        thumb_h = 4;
    }
    if (thumb_h > bounds.h) {
        thumb_h = bounds.h;
    }
    const int max_start = count - visible;
    int thumb_y = bounds.y;
    if (max_start > 0) {
        thumb_y = bounds.y + (start * (bounds.h - thumb_h)) / max_start;
    }
    display.fillRect({bounds.x, thumb_y, bounds.w, thumb_h}, palette.primary_text);
}

void drawAppCard(DisplaySurface& display, const theme::Palette& palette, int column, int row,
                 const char* name, Color color, bool selected) {
    const Rect bounds = layout::appCardBounds(column, row);
    display.fillRoundRect(bounds, layout::kCardRadius, selected ? color : palette.canvas);
    display.drawRoundRect(bounds, layout::kCardRadius, color);

    const Rect icon{bounds.x + 3, bounds.y + 3, layout::kCardIconSize, layout::kCardIconSize};
    display.fillRoundRect(icon, layout::kCardIconRadius, selected ? palette.canvas : color);

    char initial[2] = {'?', '\0'};
    if (name != nullptr && name[0] != '\0') {
        initial[0] = name[0];
        if (initial[0] >= 'a' && initial[0] <= 'z') {
            initial[0] = static_cast<char>(initial[0] - 'a' + 'A');
        }
    }

    const Color letter_color = selected ? color : palette.canvas;
    const Color name_color = selected ? palette.canvas : palette.primary_text;
    const int letter_w = font::glyphWidth(true);
    display.drawText({icon.x + (icon.w - letter_w) / 2, icon.y + (icon.h - font::kGlyphHeight) / 2},
                     {letter_color, 1, true}, initial);
    display.drawText({bounds.x + 23, centeredY(bounds.y, bounds.h)}, {name_color, 1},
                     name != nullptr ? name : "");
}

}  // namespace luma
