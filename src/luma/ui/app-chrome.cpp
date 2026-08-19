#include "luma/ui/app-chrome.h"

#include "luma/assets/luma-logo-header.h"
#include "luma/core/app-context.h"
#include "luma/core/battery.h"
#include "luma/core/clock.h"
#include "luma/core/network.h"
#include "luma/core/settings.h"
#include "luma/ui/components.h"
#include "luma/ui/renderer.h"

namespace luma {

void drawStandardHeader(AppContext& context, UiRenderer& renderer, const char* title) {
    char time_label[8] = {};
    formatCivilTime(context.clock().localTime(), time_label, sizeof(time_label));
    drawAppHeader(renderer.surface(), renderer.palette(), assets::kLogoHeader, title, time_label,
                  context.network().state(), context.network().signalStrength(),
                  context.battery().current());
}

void drawStandardFooter(UiRenderer& renderer, const KeyHint* hints, int count) {
    drawFooterHints(renderer.surface(), renderer.palette(), hints, count);
}

}  // namespace luma
