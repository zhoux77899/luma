#pragma once

#include <cstdint>

namespace luma {

struct Color {
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

struct Point {
    int x;
    int y;
};

struct Rect {
    int x;
    int y;
    int w;
    int h;
};

struct TextStyle {
    Color color;
    int size;
    bool bold;

    TextStyle(Color color, int size, bool bold = false) : color(color), size(size), bold(bold) {}
};

class DisplaySurface {
public:
    virtual ~DisplaySurface() = default;
    virtual void begin() {}
    virtual int width() const = 0;
    virtual int height() const = 0;
    virtual void beginFrame() = 0;
    virtual void clear(Color color) = 0;
    virtual void fillRect(Rect rect, Color color) = 0;
    virtual void drawRect(Rect rect, Color color) = 0;
    virtual void fillRoundRect(Rect rect, int radius, Color color) = 0;
    virtual void drawRoundRect(Rect rect, int radius, Color color) = 0;
    virtual void drawText(Point origin, TextStyle style, const char* text) = 0;
    virtual void drawBitmap(Point origin, int width, int height, const uint16_t* rgb565) = 0;
    virtual void endFrame() = 0;
};

inline bool colorsEqual(Color left, Color right) {
    return left.r == right.r && left.g == right.g && left.b == right.b;
}

inline bool rectsEqual(Rect left, Rect right) {
    return left.x == right.x && left.y == right.y && left.w == right.w && left.h == right.h;
}

}  // namespace luma
