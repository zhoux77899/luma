#include "luma/core/app-context.h"

namespace luma {

AppContext::AppContext(DisplaySurface& display, Settings& settings, Storage& storage, Clock& clock)
    : display_(display), settings_(settings), storage_(storage), clock_(clock) {}

DisplaySurface& AppContext::display() { return display_; }
Settings& AppContext::settings() { return settings_; }
Storage& AppContext::storage() { return storage_; }
Clock& AppContext::clock() { return clock_; }

void AppContext::requestRedraw() { redraw_requested_ = true; }

bool AppContext::takeRedrawRequest() {
    const bool requested = redraw_requested_;
    redraw_requested_ = false;
    return requested;
}

}  // namespace luma
