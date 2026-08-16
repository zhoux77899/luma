#pragma once

#include "luma/core/display.h"
#include "luma/ui/theme.h"

namespace luma {

class UiRenderer {
public:
    explicit UiRenderer(DisplaySurface& display) : display_(display) {}

    DisplaySurface& surface() { return display_; }

    void beginFrame() { display_.beginFrame(); }
    void clearAppCanvas() { display_.clear(theme::kCanvas); }
    void endFrame() { display_.endFrame(); }

private:
    DisplaySurface& display_;
};

}  // namespace luma
