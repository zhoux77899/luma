#include "luma/ui/components.h"

#include "luma/ui/font.h"
#include "luma/ui/layout.h"
#include "luma/ui/theme.h"

#include <cstdio>

namespace luma {
namespace {

int headerTextY() { return (layout::kHeaderHeight - font::kGlyphHeight) / 2; }

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

void drawTitleHeader(DisplaySurface& display, const char* title) {
    display.fillRect(layout::kHeader, theme::kCanvas);
    display.drawText({layout::kChromeInset, headerTextY()}, {theme::kPrimaryText, 1},
                     title != nullptr ? title : "");
}

void drawLauncherHeader(DisplaySurface& display, const uint16_t* logo, const char* time) {
    display.fillRect(layout::kHeader, theme::kCanvas);
    if (logo != nullptr) {
        display.drawBitmap({layout::kHeaderLogoX, layout::kHeaderLogoY}, layout::kHeaderLogoSize,
                           layout::kHeaderLogoSize, logo);
    }
    const char* label = time != nullptr ? time : "--:--";
    const int time_x = layout::kWidth - layout::kChromeInset - font::textWidth(label, 1);
    display.drawText({time_x, headerTextY()}, {theme::kPrimaryText, 1}, label);
}

void drawMenuItem(DisplaySurface& display, Rect bounds, const char* label, bool selected) {
    display.fillRect(bounds, theme::kCanvas);
    if (selected) {
        display.drawRect(bounds, theme::kAccent);
    }
    display.drawText({bounds.x + 4, centeredY(bounds.y, bounds.h)},
                     {selected ? theme::kPrimaryText : theme::kSecondaryText, 1},
                     label != nullptr ? label : "");
}

void drawList(DisplaySurface& display, const char* const* items, int count, int selected) {
    if (items == nullptr) {
        return;
    }
    for (int i = 0; i < count; ++i) {
        const Rect row{8, layout::kContentBoth.y + 4 + i * 16, 224, 14};
        drawMenuItem(display, row, items[i], i == selected);
    }
}

void drawDialog(DisplaySurface& display, const char* title, const char* body) {
    const Rect box{30, 30, 180, 75};
    display.fillRect(box, theme::kCanvas);
    display.drawRect(box, theme::kAccent);
    display.drawText({40, 40}, {theme::kPrimaryText, 1}, title != nullptr ? title : "");
    display.drawText({40, 56}, {theme::kSecondaryText, 1}, body != nullptr ? body : "");
}

void drawKeyHint(DisplaySurface& display, const char* hint) {
    display.fillRect(layout::kFooter, theme::kCanvas);
    display.drawText({layout::kChromeInset, centeredY(layout::kFooter.y, layout::kFooter.h)},
                     {theme::kSecondaryText, 1}, hint != nullptr ? hint : "");
}

void drawAppCard(DisplaySurface& display, int column, int row, const char* name, Color color,
                 bool selected) {
    const Rect bounds = layout::appCardBounds(column, row);
    display.fillRoundRect(bounds, layout::kCardRadius, selected ? color : theme::kCanvas);
    display.drawRoundRect(bounds, layout::kCardRadius, color);

    const Rect icon{bounds.x + 3, bounds.y + 3, layout::kCardIconSize, layout::kCardIconSize};
    display.fillRoundRect(icon, layout::kCardIconRadius, selected ? theme::kCanvas : color);

    char initial[2] = {'?', '\0'};
    if (name != nullptr && name[0] != '\0') {
        initial[0] = name[0];
        if (initial[0] >= 'a' && initial[0] <= 'z') {
            initial[0] = static_cast<char>(initial[0] - 'a' + 'A');
        }
    }

    const Color letter_color = selected ? color : theme::kCanvas;
    const Color name_color = selected ? theme::kCanvas : theme::kPrimaryText;
    const int letter_w = font::glyphWidth(true);
    display.drawText({icon.x + (icon.w - letter_w) / 2, icon.y + (icon.h - font::kGlyphHeight) / 2},
                     {letter_color, 1, true}, initial);
    display.drawText({bounds.x + 23, centeredY(bounds.y, bounds.h)}, {name_color, 1},
                     name != nullptr ? name : "");
}

}  // namespace luma
