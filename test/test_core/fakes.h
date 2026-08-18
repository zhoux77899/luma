#pragma once

#include "luma/core/audio.h"
#include "luma/core/clock.h"
#include "luma/core/diagnostics.h"
#include "luma/core/display.h"
#include "luma/core/input-source.h"
#include "luma/core/in-memory-storage.h"
#include "luma/core/network.h"
#include "luma/core/storage.h"
#include "luma/core/time-zone.h"
#include "luma/core/wifi-radio.h"
#include "luma/core/app.h"
#include "luma/core/app-context.h"
#include "luma/core/settings.h"
#include "luma/luma.h"

#include <cstdio>
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

struct DrawnMono {
    Point origin;
    int width = 0;
    int height = 0;
    Color color{};
    const uint16_t* rows = nullptr;
};

class FakeDisplay : public DisplaySurface {
public:
    std::vector<std::string> texts;
    std::vector<DrawnShape> fills;
    std::vector<DrawnShape> strokes;
    std::vector<DrawnBitmap> bitmaps;
    std::vector<DrawnMono> monos;
    int draw_text_count = 0;
    bool begun = false;
    uint8_t brightness = 255;

    void begin() override { begun = true; }
    int width() const override { return 240; }
    int height() const override { return 135; }
    void beginFrame() override {
        texts.clear();
        fills.clear();
        strokes.clear();
        bitmaps.clear();
        monos.clear();
        draw_text_count = 0;
    }
    void clear(Color) override {}
    void fillRect(Rect rect, Color color) override { fills.push_back({rect, color}); }
    void drawRect(Rect rect, Color color) override { strokes.push_back({rect, color}); }
    void fillRoundRect(Rect rect, int, Color color) override { fills.push_back({rect, color}); }
    void drawRoundRect(Rect rect, int, Color color) override { strokes.push_back({rect, color}); }
    void drawText(Point, TextStyle, const char* text) override {
        ++draw_text_count;
        texts.push_back(text);
    }
    void drawBitmap(Point origin, int width, int height, const uint16_t*) override {
        bitmaps.push_back({origin, width, height});
    }
    void drawMonoBitmap(Point origin, int width, int height, const uint16_t* rows,
                        Color color) override {
        monos.push_back({origin, width, height, color, rows});
    }
    void setBrightness(uint8_t percent) override { brightness = percent; }
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

    bool hasFill(Rect rect, Color color) const {
        for (const auto& fill : fills) {
            if (rectsEqual(fill.rect, rect) && colorsEqual(fill.color, color)) {
                return true;
            }
        }
        return false;
    }

    bool hasMono(const uint16_t* rows, Color color) const {
        for (const auto& mono : monos) {
            if (mono.rows == rows && colorsEqual(mono.color, color)) {
                return true;
            }
        }
        return false;
    }

    bool hasBitmap(Point origin, int width, int height) const {
        for (const auto& bitmap : bitmaps) {
            if (bitmap.origin.x == origin.x && bitmap.origin.y == origin.y &&
                bitmap.width == width && bitmap.height == height) {
                return true;
            }
        }
        return false;
    }
};

class FakeAudio : public Audio {
public:
    bool begun = false;
    uint8_t volume = 80;
    std::vector<std::string> events;

    void begin() override { begun = true; }
    void setVolume(uint8_t percent) override { volume = percent; }
    void play(const char* event) override { events.push_back(event); }
};

class FakeClock : public Clock {
public:
    uint32_t now = 0;
    CivilTime civil{};
    bool use_unix = false;
    bool synchronized = false;
    bool ntp_succeeds = true;
    int64_t unix_utc = 0;
    char tz[40] = "UTC";
    Storage* storage = nullptr;
    int synchronize_count = 0;

    uint32_t millis() const override { return now; }

    CivilTime localTime() const override {
        if (!use_unix) {
            return civil;
        }
        if (!synchronized) {
            return {};
        }
        return civilTimeAt(unix_utc, tz);
    }

    void synchronize() override {
        ++synchronize_count;
        if (ntp_succeeds) {
            synchronized = true;
        }
    }

    void attach(Storage& attached, Diagnostics&) override { storage = &attached; }

    void setTimeZone(const char* id) override {
        std::snprintf(tz, sizeof(tz), "%s", canonicalTimeZoneId(id));
        if (storage != nullptr) {
            storage->savePref("timezone", tz, sizeof(tz));
        }
    }

    const char* timeZoneId() const override { return tz; }
};

class FakeWifiRadio : public WifiRadio {
public:
    luma::NetworkState state = luma::NetworkState::Disconnected;
    bool scan_done = false;
    std::vector<luma::WifiScanHit> hits;
    char ssid[33] = {};
    int8_t rssi_dbm = -55;
    int connect_calls = 0;
    int scan_calls = 0;
    char last_password[64] = {};

    void startScan() override {
        ++scan_calls;
        scan_done = false;
    }

    void completeScan() { scan_done = true; }

    void addHit(const char* name, bool encrypted, int8_t rssi = -60) {
        luma::WifiScanHit hit;
        std::snprintf(hit.ssid, sizeof(hit.ssid), "%s", name);
        hit.encrypted = encrypted;
        hit.rssi = rssi;
        hits.push_back(hit);
    }

    bool scanComplete() const override { return scan_done; }
    int scanCount() const override { return static_cast<int>(hits.size()); }

    bool scanAt(int index, luma::WifiScanHit& out) const override {
        if (index < 0 || index >= static_cast<int>(hits.size())) {
            return false;
        }
        out = hits[static_cast<size_t>(index)];
        return true;
    }

    void connect(const char* name, const char* password) override {
        ++connect_calls;
        std::snprintf(ssid, sizeof(ssid), "%s", name != nullptr ? name : "");
        std::snprintf(last_password, sizeof(last_password), "%s", password != nullptr ? password : "");
        state = luma::NetworkState::Connecting;
    }

    void succeed() { state = luma::NetworkState::Connected; }
    void fail() { state = luma::NetworkState::Failed; }
    void drop() { state = luma::NetworkState::Disconnected; }

    void disconnect() override {
        state = luma::NetworkState::Disconnected;
        ssid[0] = '\0';
    }

    luma::NetworkState radioState() const override { return state; }
    const char* connectedSsid() const override { return ssid; }
    int8_t rssi() const override { return rssi_dbm; }
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

class ControllableStorage : public InMemoryStorage {
public:
    bool write_succeeds = true;
    bool save_pref_succeeds = true;

    bool savePref(const char* key, const void* data, size_t size) override {
        if (!save_pref_succeeds) {
            return false;
        }
        return InMemoryStorage::savePref(key, data, size);
    }

    bool writeFileAtomic(const char* path, const char* data, size_t length) override {
        if (!write_succeeds) {
            return false;
        }
        return InMemoryStorage::writeFileAtomic(path, data, length);
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

template <typename StorageT = InMemoryStorage>
struct LumaHarness {
    FakeDisplay display;
    FakeInputSource input;
    FakeClock clock;
    StorageT storage;
    Settings settings;
    FakeDiagnostics diagnostics;
    FakeAudio audio;
    FakeWifiRadio radio;
    Network network;
    Luma luma;

    LumaHarness()
        : luma(display, input, clock, storage, settings, diagnostics, audio, network) {
        network.attach(radio, storage, diagnostics, clock);
    }
};

}  // namespace test
}  // namespace luma
