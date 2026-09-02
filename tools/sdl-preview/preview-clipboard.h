#pragma once

#include <cstdint>
#include <string>

struct SDL_Window;

namespace luma {

bool copyPreviewScreenshot(SDL_Window* window, const uint32_t* argb, int width, int height, std::string& error);
void pumpPreviewClipboard(SDL_Window* window);

}  // namespace luma
