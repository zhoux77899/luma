#include "sdl-display-adapter.h"

#include "sdl-font.h"

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

void SdlDisplayAdapter::drawGlyph(int x, int y, char character, int scale, Color color) {
    unsigned char code = static_cast<unsigned char>(character);
    if (code < 32 || code > 127) {
        code = '?';
    }
    const uint8_t* glyph = font::kGlyphs[code - 32];
    for (int row = 0; row < font::kGlyphHeight; ++row) {
        for (int column = 0; column < font::kGlyphWidth; ++column) {
            if ((glyph[row] & (1u << column)) == 0) {
                continue;
            }
            for (int dy = 0; dy < scale; ++dy) {
                for (int dx = 0; dx < scale; ++dx) {
                    setPixel(x + column * scale + dx, y + row * scale + dy, color);
                }
            }
        }
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
    if (text == nullptr) {
        return;
    }
    const int scale = style.size < 1 ? 1 : style.size;
    int x = origin.x;
    for (const char* cursor = text; *cursor != '\0'; ++cursor) {
        drawGlyph(x, origin.y, *cursor, scale, style.color);
        x += font::kGlyphWidth * scale;
    }
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

    SDL_UpdateTexture(texture_, nullptr, pixels_, kWidth * static_cast<int>(sizeof(uint32_t)));
    SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
    SDL_RenderClear(renderer_);
    SDL_RenderCopy(renderer_, texture_, nullptr, &dest);
    SDL_RenderPresent(renderer_);
}

}  // namespace luma
