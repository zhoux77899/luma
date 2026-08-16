#include "luma/ui/components.h"

#include "luma/ui/font.h"
#include "luma/ui/layout.h"

#include <cstdio>

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

void drawAppHeader(DisplaySurface& display, const theme::Palette& palette, const uint16_t* logo,
                   const char* title, const char* time) {
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
    const int time_x = layout::kWidth - layout::kChromeInset - font::textWidth(label, 1);
    display.drawText({time_x, headerTextY()}, {palette.primary_text, 1}, label);
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
