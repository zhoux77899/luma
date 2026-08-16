#include "luma/apps/about-app.h"

#include "luma/core/app-context.h"
#include "luma/core/settings.h"
#include "luma/ui/app-chrome.h"
#include "luma/ui/components.h"
#include "luma/ui/renderer.h"
#include "luma/ui/theme.h"
#include "luma/version.h"

namespace luma {

const char* AboutApp::id() const { return "about"; }
const char* AboutApp::name() const { return "ABOUT"; }

void AboutApp::onEnter(AppContext& context) { context_ = &context; }

void AboutApp::update(const InputFrame& input) { (void)input; }

void AboutApp::draw() {
    if (context_ == nullptr) {
        return;
    }

    const theme::Palette palette = theme::paletteFor(context_->settings().theme());
    UiRenderer renderer(context_->display(), palette);
    renderer.beginFrame();
    renderer.clearAppCanvas();
    drawStandardHeader(*context_, renderer, name());

    const char* items[] = {LUMA_VERSION, LUMA_HARDWARE, LUMA_BUILD_ENV, LUMA_LICENSE};
    drawList(renderer.surface(), palette, items, 4, -1);
    const KeyHint hints[] = {{"Esc", "back"}};
    drawStandardFooter(renderer, hints, 1);
    renderer.endFrame();
}

}  // namespace luma
