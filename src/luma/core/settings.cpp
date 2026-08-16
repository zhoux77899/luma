#include "luma/core/settings.h"

#include "luma/core/clock.h"
#include "luma/core/diagnostics.h"
#include "luma/core/storage.h"

namespace luma {
namespace {

bool brightnessValid(uint8_t value) { return value <= 100; }
bool volumeValid(uint8_t value) { return value <= 100; }
bool themeValid(uint8_t value) { return value <= 1; }
bool schemaValid(uint8_t value) { return value == Settings::kDefaultSchema; }

}  // namespace

void Settings::attach(Storage& storage, Diagnostics& diagnostics, Clock& clock) {
    storage_ = &storage;
    diagnostics_ = &diagnostics;
    clock_ = &clock;
}

void Settings::load() {
    uint8_t brightness = kDefaultBrightness;
    if (loadU8(kBrightnessKey, brightness) && brightnessValid(brightness)) {
        brightness_ = brightness;
    } else {
        brightness_ = kDefaultBrightness;
    }

    uint8_t volume = kDefaultVolume;
    if (loadU8(kVolumeKey, volume) && volumeValid(volume)) {
        volume_ = volume;
    } else {
        volume_ = kDefaultVolume;
    }

    uint8_t theme = kDefaultTheme;
    if (loadU8(kThemeKey, theme) && themeValid(theme)) {
        theme_ = theme;
    } else {
        theme_ = kDefaultTheme;
    }

    uint8_t schema = kDefaultSchema;
    if (loadU8(kSchemaKey, schema) && schemaValid(schema)) {
        schema_ = schema;
    } else {
        schema_ = kDefaultSchema;
    }
}

void Settings::requestFlush() { markDirty(); }

void Settings::processDeferredSaves(uint32_t now_ms) {
    if (!pending_) {
        return;
    }
    if (now_ms - last_change_ms_ < kFlushDelayMs) {
        return;
    }
    flushNow();
}

void Settings::flushNow() {
    if (!pending_) {
        return;
    }
    pending_ = false;
    if (storage_ == nullptr) {
        return;
    }

    const bool ok = saveU8(kBrightnessKey, brightness_) && saveU8(kVolumeKey, volume_) &&
                    saveU8(kThemeKey, theme_) && saveU8(kSchemaKey, schema_);
    if (diagnostics_ == nullptr) {
        return;
    }
    if (ok) {
        diagnostics_->emit("SETTINGS", "saved");
    } else {
        diagnostics_->emit("ERROR", "settings save failed");
        pending_ = true;
    }
}

uint8_t Settings::brightness() const { return brightness_; }
uint8_t Settings::volume() const { return volume_; }
uint8_t Settings::theme() const { return theme_; }
uint8_t Settings::schema() const { return schema_; }

void Settings::setBrightness(uint8_t brightness) {
    if (!brightnessValid(brightness) || brightness_ == brightness) {
        return;
    }
    brightness_ = brightness;
    markDirty();
}

void Settings::setVolume(uint8_t volume) {
    if (!volumeValid(volume) || volume_ == volume) {
        return;
    }
    volume_ = volume;
    markDirty();
}

void Settings::setTheme(uint8_t theme) {
    if (!themeValid(theme) || theme_ == theme) {
        return;
    }
    theme_ = theme;
    markDirty();
}

void Settings::markDirty() {
    pending_ = true;
    if (clock_ != nullptr) {
        last_change_ms_ = clock_->millis();
    }
}

bool Settings::loadU8(const char* key, uint8_t& value) const {
    if (storage_ == nullptr) {
        return false;
    }
    return storage_->loadPref(key, &value, sizeof(value));
}

bool Settings::saveU8(const char* key, uint8_t value) const {
    if (storage_ == nullptr) {
        return false;
    }
    return storage_->savePref(key, &value, sizeof(value));
}

}  // namespace luma
