#include "preview-menu.h"

#include "luma/ui/font.h"

#include <SDL.h>

namespace luma {
namespace {

constexpr int kFilePad = 8;
constexpr int kFileWidth = 40;
constexpr int kMenuWidth = 160;
constexpr int kItemHeight = 22;

bool g_file_open = false;

void drawText(SDL_Renderer* renderer, int x, int y, const char* text, Color color) {
    font::drawText({x, y}, TextStyle{color, 1, false}, text, [renderer](int px, int py, Color pixel) {
        SDL_SetRenderDrawColor(renderer, pixel.r, pixel.g, pixel.b, 255);
        SDL_RenderDrawPoint(renderer, px, py);
    });
}

bool inRect(int x, int y, int rx, int ry, int rw, int rh) {
    return x >= rx && y >= ry && x < rx + rw && y < ry + rh;
}

}  // namespace

void attachPreviewMenu(SDL_Window*) {}

int previewMenuTopInset() { return kPreviewMenuStripHeight; }

bool handlePreviewMenuEvent(const SDL_Event& event, PreviewCommand& command) {
    if (event.type == SDL_KEYDOWN && g_file_open && event.key.keysym.sym == SDLK_ESCAPE) {
        g_file_open = false;
        return true;
    }

    if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_FOCUS_LOST) {
        g_file_open = false;
        return false;
    }

    if (event.type != SDL_MOUSEBUTTONDOWN || event.button.button != SDL_BUTTON_LEFT) {
        return false;
    }

    const int x = event.button.x;
    const int y = event.button.y;
    if (inRect(x, y, 0, 0, kFileWidth + kFilePad * 2, kPreviewMenuStripHeight)) {
        g_file_open = !g_file_open;
        return true;
    }

    if (g_file_open && inRect(x, y, kFilePad, kPreviewMenuStripHeight, kMenuWidth, kItemHeight * 2)) {
        if (y < kPreviewMenuStripHeight + kItemHeight) {
            command = PreviewCommand::CopyScreenshot;
        } else {
            command = PreviewCommand::SaveScreenshot;
        }
        g_file_open = false;
        return true;
    }

    if (g_file_open) {
        g_file_open = false;
        return y < kPreviewMenuStripHeight;
    }
    return y < kPreviewMenuStripHeight;
}

PreviewCommand takePreviewMenuCommand() { return PreviewCommand::None; }

void drawPreviewMenuOverlay(SDL_Renderer* renderer, int window_w, int window_h) {
    (void)window_h;
    if (renderer == nullptr) {
        return;
    }

    SDL_Rect strip{0, 0, window_w, kPreviewMenuStripHeight};
    SDL_SetRenderDrawColor(renderer, 42, 42, 42, 255);
    SDL_RenderFillRect(renderer, &strip);
    SDL_SetRenderDrawColor(renderer, 70, 70, 70, 255);
    SDL_RenderDrawLine(renderer, 0, kPreviewMenuStripHeight - 1, window_w, kPreviewMenuStripHeight - 1);

    if (g_file_open) {
        SDL_Rect file_hit{0, 0, kFileWidth + kFilePad * 2, kPreviewMenuStripHeight};
        SDL_SetRenderDrawColor(renderer, 64, 64, 64, 255);
        SDL_RenderFillRect(renderer, &file_hit);
    }
    drawText(renderer, kFilePad, 9, "File", Color{236, 236, 236});

    if (!g_file_open) {
        return;
    }

    SDL_Rect menu{kFilePad, kPreviewMenuStripHeight, kMenuWidth, kItemHeight * 2};
    SDL_SetRenderDrawColor(renderer, 36, 36, 36, 255);
    SDL_RenderFillRect(renderer, &menu);
    SDL_SetRenderDrawColor(renderer, 90, 90, 90, 255);
    SDL_RenderDrawRect(renderer, &menu);
    drawText(renderer, kFilePad + 8, kPreviewMenuStripHeight + 6, "Copy Screenshot", Color{236, 236, 236});
    drawText(renderer, kFilePad + 8, kPreviewMenuStripHeight + kItemHeight + 6, "Save Screenshot...",
             Color{236, 236, 236});
}

}  // namespace luma
