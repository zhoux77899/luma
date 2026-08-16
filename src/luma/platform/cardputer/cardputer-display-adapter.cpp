#include "cardputer-display-adapter.h"

#include "luma/ui/font.h"

#include <M5Cardputer.h>

namespace luma {
namespace {

uint16_t to565(Color color) {
    return static_cast<uint16_t>(((color.r & 0xF8) << 8) | ((color.g & 0xFC) << 3) | (color.b >> 3));
}

}  // namespace

void CardputerDisplayAdapter::begin() { M5Cardputer.Display.setRotation(1); }

int CardputerDisplayAdapter::width() const { return 240; }
int CardputerDisplayAdapter::height() const { return 135; }

void CardputerDisplayAdapter::beginFrame() {}

void CardputerDisplayAdapter::clear(Color color) { M5Cardputer.Display.fillScreen(to565(color)); }

void CardputerDisplayAdapter::fillRect(Rect rect, Color color) {
    M5Cardputer.Display.fillRect(rect.x, rect.y, rect.w, rect.h, to565(color));
}

void CardputerDisplayAdapter::drawRect(Rect rect, Color color) {
    M5Cardputer.Display.drawRect(rect.x, rect.y, rect.w, rect.h, to565(color));
}

void CardputerDisplayAdapter::fillRoundRect(Rect rect, int radius, Color color) {
    M5Cardputer.Display.fillRoundRect(rect.x, rect.y, rect.w, rect.h, radius, to565(color));
}

void CardputerDisplayAdapter::drawRoundRect(Rect rect, int radius, Color color) {
    M5Cardputer.Display.drawRoundRect(rect.x, rect.y, rect.w, rect.h, radius, to565(color));
}

void CardputerDisplayAdapter::drawText(Point origin, TextStyle style, const char* text) {
    font::drawText(origin, style, text, [](int x, int y, Color color) {
        M5Cardputer.Display.drawPixel(x, y, to565(color));
    });
}

void CardputerDisplayAdapter::drawBitmap(Point origin, int width, int height, const uint16_t* rgb565) {
    if (rgb565 == nullptr || width <= 0 || height <= 0) {
        return;
    }
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const uint16_t pixel = rgb565[y * width + x];
            if (pixel == 0) {
                continue;
            }
            M5Cardputer.Display.drawPixel(origin.x + x, origin.y + y, pixel);
        }
    }
}

void CardputerDisplayAdapter::endFrame() {}

}  // namespace luma
