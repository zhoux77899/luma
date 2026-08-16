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
    virtual void drawText(Point origin, TextStyle style, const char* text) = 0;
    virtual void endFrame() = 0;
};

}  // namespace luma
