#pragma once

#include "preview-command.h"

struct SDL_Renderer;
struct SDL_Window;
union SDL_Event;

namespace luma {

void attachPreviewMenu(SDL_Window* window);
int previewMenuTopInset();
bool handlePreviewMenuEvent(const SDL_Event& event, PreviewCommand& command);
PreviewCommand takePreviewMenuCommand();
void drawPreviewMenuOverlay(SDL_Renderer* renderer, int window_w, int window_h);

}  // namespace luma
