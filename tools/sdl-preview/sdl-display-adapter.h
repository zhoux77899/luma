#pragma once

#include "luma/core/display.h"

struct SDL_Window;
struct SDL_Renderer;
struct SDL_Texture;

namespace luma {

class SdlDisplayAdapter : public DisplaySurface {
public:
    ~SdlDisplayAdapter() override;

    void begin() override;
    int width() const override;
    int height() const override;
    void beginFrame() override;
    void clear(Color color) override;
    void fillRect(Rect rect, Color color) override;
    void drawRect(Rect rect, Color color) override;
    void drawText(Point origin, TextStyle style, const char* text) override;
    void endFrame() override;

private:
    void setPixel(int x, int y, Color color);
    void drawGlyph(int x, int y, char character, int scale, Color color);

    SDL_Window* window_ = nullptr;
    SDL_Renderer* renderer_ = nullptr;
    SDL_Texture* texture_ = nullptr;
    uint32_t pixels_[240 * 135]{};
};

}  // namespace luma
