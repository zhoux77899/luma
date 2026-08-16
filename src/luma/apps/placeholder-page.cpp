#include "luma/apps/placeholder-page.h"

#include "luma/core/app-context.h"
#include "luma/core/display.h"
#include "luma/ui/components.h"
#include "luma/ui/renderer.h"

namespace luma {

void drawPlaceholderPage(AppContext& context, const char* title) {
    UiRenderer renderer(context.display());
    renderer.beginFrame();
    renderer.clearAppCanvas();
    drawTitleHeader(renderer.surface(), title);
    const char* items[] = {"Coming soon"};
    drawList(renderer.surface(), items, 1, 0);
    drawKeyHint(renderer.surface(), "Esc Back");
    renderer.endFrame();
}

}  // namespace luma
