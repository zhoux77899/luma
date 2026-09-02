#pragma once

#include "luma/core/display.h"

#include <cstdint>
#include <vector>

struct SDL_Window;
struct SDL_Renderer;
struct SDL_Texture;
struct SDL_Rect;

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
    void fillRoundRect(Rect rect, int radius, Color color) override;
    void drawRoundRect(Rect rect, int radius, Color color) override;
    void drawText(Point origin, TextStyle style, const char* text) override;
    void drawBitmap(Point origin, int width, int height, const uint16_t* rgb565) override;
    void setBrightness(uint8_t percent) override;
    void endFrame() override;

    SDL_Window* window() const { return window_; }
    int currentScale() const;
    void copyPresentedArgb(std::vector<uint32_t>& dest, int& width, int& height) const;

private:
    void setPixel(int x, int y, Color color);
    uint32_t dimPixel(uint32_t pixel) const;
    void canvasDest(SDL_Rect& dest) const;

    SDL_Window* window_ = nullptr;
    SDL_Renderer* renderer_ = nullptr;
    SDL_Texture* texture_ = nullptr;
    uint32_t pixels_[240 * 135]{};
    uint8_t brightness_ = 80;
    int top_inset_ = 0;
};

}  // namespace luma
