#include "sdl-display-adapter.h"

#include "luma/ui/font.h"

#include <SDL.h>

#include <algorithm>

namespace luma {
namespace {

constexpr int kWidth = 240;
constexpr int kHeight = 135;
constexpr int kWindowWidth = 960;
constexpr int kWindowHeight = 540;

uint32_t pack(Color color) {
    return (255u << 24) | (static_cast<uint32_t>(color.r) << 16) |
           (static_cast<uint32_t>(color.g) << 8) | static_cast<uint32_t>(color.b);
}

}  // namespace

SdlDisplayAdapter::~SdlDisplayAdapter() {
    if (texture_ != nullptr) {
        SDL_DestroyTexture(texture_);
    }
    if (renderer_ != nullptr) {
        SDL_DestroyRenderer(renderer_);
    }
    if (window_ != nullptr) {
        SDL_DestroyWindow(window_);
    }
}

void SdlDisplayAdapter::begin() {
    if (window_ != nullptr) {
        return;
    }

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
    window_ = SDL_CreateWindow("Luma SDL preview", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                               kWindowWidth, kWindowHeight, SDL_WINDOW_RESIZABLE);
    if (window_ == nullptr) {
        return;
    }

    renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (renderer_ == nullptr) {
        renderer_ = SDL_CreateRenderer(window_, -1, 0);
    }
    if (renderer_ == nullptr) {
        return;
    }

    texture_ = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING,
                                 kWidth, kHeight);
}

int SdlDisplayAdapter::width() const { return kWidth; }

int SdlDisplayAdapter::height() const { return kHeight; }

void SdlDisplayAdapter::beginFrame() {}

void SdlDisplayAdapter::setPixel(int x, int y, Color color) {
    if (x < 0 || y < 0 || x >= kWidth || y >= kHeight) {
        return;
    }
    pixels_[y * kWidth + x] = pack(color);
}

void SdlDisplayAdapter::clear(Color color) {
    const uint32_t pixel = pack(color);
    for (uint32_t& entry : pixels_) {
        entry = pixel;
    }
}

void SdlDisplayAdapter::fillRect(Rect rect, Color color) {
    const int x0 = std::max(rect.x, 0);
    const int y0 = std::max(rect.y, 0);
    const int x1 = std::min(rect.x + rect.w, kWidth);
    const int y1 = std::min(rect.y + rect.h, kHeight);
    for (int y = y0; y < y1; ++y) {
        for (int x = x0; x < x1; ++x) {
            setPixel(x, y, color);
        }
    }
}

void SdlDisplayAdapter::fillRoundRect(Rect rect, int radius, Color color) {
    int r = radius;
    if (r < 0) {
        r = 0;
    }
    if (r * 2 > rect.w) {
        r = rect.w / 2;
    }
    if (r * 2 > rect.h) {
        r = rect.h / 2;
    }
    if (r <= 0) {
        fillRect(rect, color);
        return;
    }

    fillRect({rect.x + r, rect.y, rect.w - 2 * r, rect.h}, color);
    fillRect({rect.x, rect.y + r, r, rect.h - 2 * r}, color);
    fillRect({rect.x + rect.w - r, rect.y + r, r, rect.h - 2 * r}, color);

    const int r2 = r * r;
    for (int dy = 0; dy < r; ++dy) {
        for (int dx = 0; dx < r; ++dx) {
            if (dx * dx + dy * dy > r2) {
                continue;
            }
            setPixel(rect.x + r - 1 - dx, rect.y + r - 1 - dy, color);
            setPixel(rect.x + rect.w - r + dx, rect.y + r - 1 - dy, color);
            setPixel(rect.x + r - 1 - dx, rect.y + rect.h - r + dy, color);
            setPixel(rect.x + rect.w - r + dx, rect.y + rect.h - r + dy, color);
        }
    }
}

void SdlDisplayAdapter::drawRoundRect(Rect rect, int radius, Color color) {
    int r = radius;
    if (r < 0) {
        r = 0;
    }
    if (r * 2 > rect.w) {
        r = rect.w / 2;
    }
    if (r * 2 > rect.h) {
        r = rect.h / 2;
    }
    if (r <= 0) {
        drawRect(rect, color);
        return;
    }

    for (int x = rect.x + r; x < rect.x + rect.w - r; ++x) {
        setPixel(x, rect.y, color);
        setPixel(x, rect.y + rect.h - 1, color);
    }
    for (int y = rect.y + r; y < rect.y + rect.h - r; ++y) {
        setPixel(rect.x, y, color);
        setPixel(rect.x + rect.w - 1, y, color);
    }

    const int r2 = r * r;
    const int inner = (r - 1) * (r - 1);
    for (int dy = 0; dy < r; ++dy) {
        for (int dx = 0; dx < r; ++dx) {
            const int d2 = dx * dx + dy * dy;
            if (d2 > r2 || d2 < inner) {
                continue;
            }
            setPixel(rect.x + r - 1 - dx, rect.y + r - 1 - dy, color);
            setPixel(rect.x + rect.w - r + dx, rect.y + r - 1 - dy, color);
            setPixel(rect.x + r - 1 - dx, rect.y + rect.h - r + dy, color);
            setPixel(rect.x + rect.w - r + dx, rect.y + rect.h - r + dy, color);
        }
    }
}

void SdlDisplayAdapter::drawRect(Rect rect, Color color) {
    if (rect.w <= 0 || rect.h <= 0) {
        return;
    }
    for (int x = rect.x; x < rect.x + rect.w; ++x) {
        setPixel(x, rect.y, color);
        setPixel(x, rect.y + rect.h - 1, color);
    }
    for (int y = rect.y; y < rect.y + rect.h; ++y) {
        setPixel(rect.x, y, color);
        setPixel(rect.x + rect.w - 1, y, color);
    }
}

void SdlDisplayAdapter::drawBitmap(Point origin, int width, int height, const uint16_t* rgb565) {
    if (rgb565 == nullptr || width <= 0 || height <= 0) {
        return;
    }
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const uint16_t pixel = rgb565[y * width + x];
            if (pixel == 0) {
                continue;
            }
            Color color;
            color.r = static_cast<uint8_t>((pixel >> 11) << 3);
            color.g = static_cast<uint8_t>(((pixel >> 5) & 0x3F) << 2);
            color.b = static_cast<uint8_t>((pixel & 0x1F) << 3);
            setPixel(origin.x + x, origin.y + y, color);
        }
    }
}

void SdlDisplayAdapter::drawText(Point origin, TextStyle style, const char* text) {
    font::drawText(origin, style, text, [this](int x, int y, Color color) { setPixel(x, y, color); });
}

void SdlDisplayAdapter::setBrightness(uint8_t percent) {
    brightness_ = percent > 100 ? 100 : percent;
}

uint32_t SdlDisplayAdapter::dimPixel(uint32_t pixel) const {
    if (brightness_ >= 100) {
        return pixel;
    }
    const uint32_t r = ((pixel >> 16) & 0xFFu) * brightness_ / 100u;
    const uint32_t g = ((pixel >> 8) & 0xFFu) * brightness_ / 100u;
    const uint32_t b = (pixel & 0xFFu) * brightness_ / 100u;
    return (255u << 24) | (r << 16) | (g << 8) | b;
}

void SdlDisplayAdapter::endFrame() {
    if (renderer_ == nullptr || texture_ == nullptr) {
        return;
    }

    int window_w = 0;
    int window_h = 0;
    SDL_GetWindowSize(window_, &window_w, &window_h);
    int scale = window_w / kWidth;
    const int scale_h = window_h / kHeight;
    if (scale_h < scale) {
        scale = scale_h;
    }
    if (scale < 1) {
        scale = 1;
    }

    SDL_Rect dest;
    dest.w = kWidth * scale;
    dest.h = kHeight * scale;
    dest.x = (window_w - dest.w) / 2;
    dest.y = (window_h - dest.h) / 2;

    uint32_t framed[kWidth * kHeight];
    for (int i = 0; i < kWidth * kHeight; ++i) {
        framed[i] = dimPixel(pixels_[i]);
    }

    SDL_UpdateTexture(texture_, nullptr, framed, kWidth * static_cast<int>(sizeof(uint32_t)));
    SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
    SDL_RenderClear(renderer_);
    SDL_RenderCopy(renderer_, texture_, nullptr, &dest);
    SDL_RenderPresent(renderer_);
}

}  // namespace luma
