#include "luma/core/settings.h"

namespace luma {

void Settings::load() {
    if (!valid()) {
        applyDefaults();
    }
}

void Settings::requestFlush() { flush_requested_ = true; }

void Settings::processDeferredSaves() {
    if (!flush_requested_) {
        return;
    }
    flush_requested_ = false;
}

uint8_t Settings::brightness() const { return brightness_; }
bool Settings::sound() const { return sound_; }
uint8_t Settings::theme() const { return theme_; }
uint8_t Settings::schema() const { return schema_; }

void Settings::setBrightness(uint8_t brightness) {
    brightness_ = brightness;
    if (!valid()) {
        brightness_ = kDefaultBrightness;
    }
}

void Settings::setSound(bool sound) { sound_ = sound; }

void Settings::setTheme(uint8_t theme) {
    theme_ = theme;
    if (!valid()) {
        theme_ = kDefaultTheme;
    }
}

void Settings::applyDefaults() {
    brightness_ = kDefaultBrightness;
    sound_ = kDefaultSound;
    theme_ = kDefaultTheme;
    schema_ = kDefaultSchema;
}

bool Settings::valid() const {
    return brightness_ <= 100 && theme_ <= 1 && schema_ == kDefaultSchema;
}

}  // namespace luma
