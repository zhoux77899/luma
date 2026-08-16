#pragma once

#include "luma/core/display.h"
#include "luma/ui/theme.h"

namespace luma {

class UiRenderer {
public:
    UiRenderer(DisplaySurface& display, const theme::Palette& palette)
        : display_(display), palette_(palette) {}

    DisplaySurface& surface() { return display_; }
    const theme::Palette& palette() const { return palette_; }

    void beginFrame() { display_.beginFrame(); }
    void clearAppCanvas() { display_.clear(palette_.canvas); }
    void endFrame() { display_.endFrame(); }

private:
    DisplaySurface& display_;
    theme::Palette palette_;
};

}  // namespace luma
