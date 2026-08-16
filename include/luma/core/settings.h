#pragma once

#include <cstdint>

namespace luma {

class Settings {
public:
    static constexpr uint8_t kDefaultBrightness = 80;
    static constexpr bool kDefaultSound = true;
    static constexpr uint8_t kDefaultTheme = 0;
    static constexpr uint8_t kDefaultSchema = 1;

    void load();
    void requestFlush();
    void processDeferredSaves();

    uint8_t brightness() const;
    bool sound() const;
    uint8_t theme() const;
    uint8_t schema() const;

    void setBrightness(uint8_t brightness);
    void setSound(bool sound);
    void setTheme(uint8_t theme);

private:
    void applyDefaults();
    bool valid() const;

    uint8_t brightness_ = kDefaultBrightness;
    bool sound_ = kDefaultSound;
    uint8_t theme_ = kDefaultTheme;
    uint8_t schema_ = kDefaultSchema;
    bool flush_requested_ = false;
};

}  // namespace luma
