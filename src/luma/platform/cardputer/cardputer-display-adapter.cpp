#include "cardputer-display-adapter.h"

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

void CardputerDisplayAdapter::drawText(Point origin, TextStyle style, const char* text) {
    M5Cardputer.Display.setTextSize(style.size);
    M5Cardputer.Display.setTextColor(to565(style.color));
    M5Cardputer.Display.setCursor(origin.x, origin.y);
    M5Cardputer.Display.print(text);
}

void CardputerDisplayAdapter::endFrame() {}

}  // namespace luma
