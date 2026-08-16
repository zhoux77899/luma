#pragma once

#include <cstdint>

namespace luma {

class Clock;
class Diagnostics;
class Storage;

class Settings {
public:
    static constexpr uint8_t kDefaultBrightness = 80;
    static constexpr bool kDefaultSound = true;
    static constexpr uint8_t kDefaultTheme = 0;
    static constexpr uint8_t kDefaultSchema = 1;
    static constexpr uint32_t kFlushDelayMs = 500;
    static constexpr const char* kBrightnessKey = "brightness";
    static constexpr const char* kSoundKey = "sound";
    static constexpr const char* kThemeKey = "theme";
    static constexpr const char* kSchemaKey = "schema";

    void attach(Storage& storage, Diagnostics& diagnostics, Clock& clock);
    void load();
    void requestFlush();
    void processDeferredSaves(uint32_t now_ms);
    void flushNow();

    uint8_t brightness() const;
    bool sound() const;
    uint8_t theme() const;
    uint8_t schema() const;

    void setBrightness(uint8_t brightness);
    void setSound(bool sound);
    void setTheme(uint8_t theme);

private:
    void markDirty();
    bool loadU8(const char* key, uint8_t& value) const;
    bool saveU8(const char* key, uint8_t value) const;

    Storage* storage_ = nullptr;
    Diagnostics* diagnostics_ = nullptr;
    Clock* clock_ = nullptr;
    uint8_t brightness_ = kDefaultBrightness;
    bool sound_ = kDefaultSound;
    uint8_t theme_ = kDefaultTheme;
    uint8_t schema_ = kDefaultSchema;
    bool pending_ = false;
    uint32_t last_change_ms_ = 0;
};

}  // namespace luma
