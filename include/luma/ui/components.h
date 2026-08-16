#pragma once

#include "luma/core/clock.h"
#include "luma/core/display.h"

namespace luma {

void formatCivilTime(const CivilTime& time, char* out, unsigned out_size);

void drawTitleHeader(DisplaySurface& display, const char* title);
void drawLauncherHeader(DisplaySurface& display, const uint16_t* logo, const char* time);
void drawMenuItem(DisplaySurface& display, Rect bounds, const char* label, bool selected);
void drawList(DisplaySurface& display, const char* const* items, int count, int selected);
void drawDialog(DisplaySurface& display, const char* title, const char* body);
void drawKeyHint(DisplaySurface& display, const char* hint);
void drawAppCard(DisplaySurface& display, int column, int row, const char* name, bool selected);

}  // namespace luma
