#include "luma/core/app-context.h"

#include "luma/core/battery.h"
#include "luma/core/network.h"

namespace luma {

AppContext::AppContext(DisplaySurface& display, Settings& settings, Storage& storage, Clock& clock,
                       Diagnostics& diagnostics, Network& network, Battery& battery)
    : display_(display),
      settings_(settings),
      storage_(storage),
      clock_(clock),
      diagnostics_(diagnostics),
      network_(network),
      battery_(battery) {}

DisplaySurface& AppContext::display() { return display_; }
Settings& AppContext::settings() { return settings_; }
Storage& AppContext::storage() { return storage_; }
Clock& AppContext::clock() { return clock_; }
Diagnostics& AppContext::diagnostics() { return diagnostics_; }
Network& AppContext::network() { return network_; }
Battery& AppContext::battery() { return battery_; }

void AppContext::requestRedraw() { redraw_requested_ = true; }

bool AppContext::takeRedrawRequest() {
    const bool requested = redraw_requested_;
    redraw_requested_ = false;
    return requested;
}

void AppContext::requestEnter(const char* id) { enter_id_ = id; }

const char* AppContext::takeEnterRequest() {
    const char* id = enter_id_;
    enter_id_ = nullptr;
    return id;
}

void AppContext::requestUiSound() { ui_sound_requested_ = true; }

bool AppContext::takeUiSound() {
    const bool requested = ui_sound_requested_;
    ui_sound_requested_ = false;
    return requested;
}

void AppContext::consumeBack() { back_consumed_ = true; }

bool AppContext::takeBackConsumed() {
    const bool consumed = back_consumed_;
    back_consumed_ = false;
    return consumed;
}

}  // namespace luma
