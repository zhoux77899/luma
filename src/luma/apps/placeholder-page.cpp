#include "luma/apps/placeholder-page.h"

#include "luma/core/app-context.h"
#include "luma/core/settings.h"
#include "luma/ui/app-chrome.h"
#include "luma/ui/components.h"
#include "luma/ui/renderer.h"
#include "luma/ui/theme.h"

namespace luma {

void drawPlaceholderPage(AppContext& context, const char* title) {
    const theme::Palette palette = theme::paletteFor(context.settings().theme());
    UiRenderer renderer(context.display(), palette);
    renderer.beginFrame();
    renderer.clearAppCanvas();
    drawStandardHeader(context, renderer, title);
    const char* items[] = {"Coming soon"};
    drawList(renderer.surface(), palette, items, 1, 0);
    const KeyHint hints[] = {{"Esc", "back"}};
    drawStandardFooter(renderer, hints, 1);
    renderer.endFrame();
}

}  // namespace luma
