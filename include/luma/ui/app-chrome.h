#pragma once

#include "luma/ui/components.h"

namespace luma {

class AppContext;
class UiRenderer;

void drawStandardHeader(AppContext& context, UiRenderer& renderer, const char* title);
void drawStandardFooter(UiRenderer& renderer, const KeyHint* hints, int count);

}  // namespace luma
