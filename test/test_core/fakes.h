#pragma once

#include "luma/core/audio.h"
#include "luma/core/clock.h"
#include "luma/core/diagnostics.h"
#include "luma/core/display.h"
#include "luma/core/input-source.h"
#include "luma/core/storage.h"
#include "luma/core/app.h"
#include "luma/core/app-context.h"

#include <cstring>
#include <string>
#include <vector>

namespace luma {
namespace test {

class FakeDiagnostics : public Diagnostics {
public:
    std::vector<std::string> lines;

    void emit(const char* prefix, const char* message) override {
        lines.push_back(std::string("[") + prefix + "] " + message);
    }

    bool contains(const char* line) const {
        for (const auto& entry : lines) {
            if (entry == line) {
                return true;
            }
        }
        return false;
    }
};

struct DrawnShape {
    Rect rect;
    Color color;
};

struct DrawnBitmap {
    Point origin;
    int width = 0;
    int height = 0;
};

class FakeDisplay : public DisplaySurface {
public:
    std::vector<std::string> texts;
    std::vector<DrawnShape> fills;
    std::vector<DrawnShape> strokes;
    std::vector<DrawnBitmap> bitmaps;
    int draw_text_count = 0;
    bool begun = false;

    void begin() override { begun = true; }
    int width() const override { return 240; }
    int height() const override { return 135; }
    void beginFrame() override {
        texts.clear();
        fills.clear();
        strokes.clear();
        bitmaps.clear();
        draw_text_count = 0;
    }
    void clear(Color) override {}
    void fillRect(Rect rect, Color color) override { fills.push_back({rect, color}); }
    void drawRect(Rect rect, Color color) override { strokes.push_back({rect, color}); }
    void drawText(Point, TextStyle, const char* text) override {
        ++draw_text_count;
        texts.push_back(text);
    }
    void drawBitmap(Point origin, int width, int height, const uint16_t*) override {
        bitmaps.push_back({origin, width, height});
    }
    void endFrame() override {}

    bool hasText(const char* text) const {
        for (const auto& entry : texts) {
            if (entry == text) {
                return true;
            }
        }
        return false;
    }

    bool hasStroke(Rect rect, Color color) const {
        for (const auto& stroke : strokes) {
            if (rectsEqual(stroke.rect, rect) && colorsEqual(stroke.color, color)) {
                return true;
            }
        }
        return false;
    }
};

class FakeAudio : public Audio {
public:
    bool begun = false;
    std::vector<std::string> events;

    void begin() override { begun = true; }
    void play(const char* event) override { events.push_back(event); }
};

class FakeClock : public Clock {
public:
    uint32_t now = 0;
    CivilTime civil{};

    uint32_t millis() const override { return now; }
    CivilTime localTime() const override { return civil; }
};

class FakeInputSource : public InputSource {
public:
    std::vector<InputFrame> frames;
    size_t next = 0;

    void push(const InputFrame& frame) { frames.push_back(frame); }

    bool poll(InputFrame& frame) override {
        if (next >= frames.size()) {
            return false;
        }
        frame = frames[next++];
        return true;
    }
};

class CountingStorage : public Storage {
public:
    int flush_count = 0;
    bool begun = false;

    bool begin() override {
        begun = true;
        return true;
    }
    bool loadPref(const char*, void*, size_t) override { return false; }
    bool savePref(const char*, const void*, size_t) override { return true; }
    bool readFile(const char*, char*, size_t, size_t&) override { return false; }
    bool writeFileAtomic(const char*, const char*, size_t) override { return true; }
    void processDeferredSaves() override { ++flush_count; }
};

class RecordingApp : public App {
public:
    RecordingApp(const char* id, const char* name, char shortcut, std::vector<std::string>& log)
        : id_(id), name_(name), shortcut_(shortcut), log_(log) {}

    const char* id() const override { return id_; }
    const char* name() const override { return name_; }
    char shortcut() const override { return shortcut_; }

    void onEnter(AppContext& context) override {
        context_ = &context;
        log_.push_back(std::string(id_) + ":enter");
    }

    void onExit() override { log_.push_back(std::string(id_) + ":exit"); }

    void update(const InputFrame& input) override {
        last_input_ = input;
        ++update_count;
        log_.push_back(std::string(id_) + ":update");
    }

    void draw() override {
        ++draw_count;
        log_.push_back(std::string(id_) + ":draw");
    }

    void requestRedraw() {
        if (context_ != nullptr) {
            context_->requestRedraw();
        }
    }

    AppContext* context_ = nullptr;
    InputFrame last_input_{};
    int update_count = 0;
    int draw_count = 0;

private:
    const char* id_;
    const char* name_;
    char shortcut_;
    std::vector<std::string>& log_;
};

inline InputFrame makeAction(InputAction action) {
    InputFrame frame;
    frame.action = action;
    frame.pressed = true;
    return frame;
}

inline InputFrame makeText(char character) {
    InputFrame frame;
    frame.text[0] = character;
    frame.text[1] = '\0';
    frame.textLength = 1;
    frame.pressed = true;
    return frame;
}

}  // namespace test
}  // namespace luma
