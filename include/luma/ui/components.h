#pragma once

#include "luma/core/clock.h"
#include "luma/core/display.h"
#include "luma/core/network-types.h"
#include "luma/ui/theme.h"

#include <cstdint>

namespace luma {

void formatCivilTime(const CivilTime& time, char* out, unsigned out_size);

void drawNetworkGlyph(DisplaySurface& display, const theme::Palette& palette, Point origin,
                      NetworkState state, SignalStrength strength);
void drawAppHeader(DisplaySurface& display, const theme::Palette& palette, const uint16_t* logo,
                   const char* title, const char* time, NetworkState state = NetworkState::Disconnected,
                   SignalStrength strength = SignalStrength::None);
void drawMenuItem(DisplaySurface& display, const theme::Palette& palette, Rect bounds,
                  const char* label, bool selected, const char* value = nullptr,
                  bool focused = true);
void drawProgressBar(DisplaySurface& display, const theme::Palette& palette, Rect bounds,
                     uint8_t percent);
void drawList(DisplaySurface& display, const theme::Palette& palette, const char* const* items,
              int count, int selected);
void drawDialog(DisplaySurface& display, const theme::Palette& palette, const char* title,
                const char* body);

struct KeyHint {
    const char* key;
    const char* label;
};

void drawFooterHints(DisplaySurface& display, const theme::Palette& palette, const KeyHint* hints,
                     int count);
void drawOverflowScrollbar(DisplaySurface& display, const theme::Palette& palette, Rect bounds,
                           int count, int start, int visible);
void drawAppCard(DisplaySurface& display, const theme::Palette& palette, int column, int row,
                 const char* name, Color color, bool selected);

}  // namespace luma
