#include "luma/ui/components.h"

#include "luma/ui/layout.h"
#include "luma/ui/theme.h"

#include <cstdio>

namespace luma {

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
    display.drawText({6, 8}, {theme::kPrimaryText, 1}, title != nullptr ? title : "");
}

void drawLauncherHeader(DisplaySurface& display, const uint16_t* logo, const char* time) {
    display.fillRect(layout::kHeader, theme::kCanvas);
    if (logo != nullptr) {
        display.drawBitmap({2, 2}, layout::kHeaderLogoSize, layout::kHeaderLogoSize, logo);
    }
    display.drawText({198, 8}, {theme::kPrimaryText, 1}, time != nullptr ? time : "--:--");
}

void drawMenuItem(DisplaySurface& display, Rect bounds, const char* label, bool selected) {
    display.fillRect(bounds, theme::kCanvas);
    if (selected) {
        display.drawRect(bounds, theme::kAccent);
    }
    display.drawText({bounds.x + 4, bounds.y + 3},
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
    display.drawText({6, 123}, {theme::kSecondaryText, 1}, hint != nullptr ? hint : "");
}

void drawAppCard(DisplaySurface& display, int column, int row, const char* name, bool selected) {
    const Rect bounds = layout::appCardBounds(column, row);
    display.fillRect(bounds, theme::kCanvas);
    display.drawRect(bounds, selected ? theme::kAccent : theme::kSecondaryText);
    display.drawText({bounds.x + 6, bounds.y + 7}, {theme::kPrimaryText, 1},
                     name != nullptr ? name : "");
}

}  // namespace luma
