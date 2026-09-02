#pragma once

struct SDL_Window;

namespace luma {

void reportPreviewError(SDL_Window* window, const char* message);

}  // namespace luma
