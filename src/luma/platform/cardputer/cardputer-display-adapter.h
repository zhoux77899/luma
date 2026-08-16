#pragma once

#include "luma/core/display.h"

namespace luma {

class CardputerDisplayAdapter : public DisplaySurface {
public:
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
    void endFrame() override;
};

}  // namespace luma
