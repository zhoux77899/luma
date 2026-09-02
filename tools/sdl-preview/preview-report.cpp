#include "preview-report.h"

#include <SDL.h>

#include <cstdio>

namespace luma {

void reportPreviewError(SDL_Window* window, const char* message) {
    if (message == nullptr) {
        message = "Preview screenshot failed";
    }
    std::fprintf(stderr, "[ERROR] %s\n", message);
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Luma SDL preview", message, window);
}

}  // namespace luma
