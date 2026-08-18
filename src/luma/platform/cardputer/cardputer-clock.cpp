#include "cardputer-clock.h"

#include "luma/core/diagnostics.h"
#include "luma/core/storage.h"
#include "luma/core/time-zone.h"

#include <Arduino.h>
#include <cstdio>
#include <cstring>
#include <ctime>

#include "esp_sntp.h"

namespace luma {
namespace {

constexpr const char* kTimeZoneKey = "timezone";
constexpr uint32_t kNtpRefreshMs = 24ul * 60ul * 60ul * 1000ul;
constexpr int64_t kMinValidUnix = 1704067200;  // 2024-01-01

}  // namespace

uint32_t CardputerClock::millis() const { return ::millis(); }

void CardputerClock::attach(Storage& storage, Diagnostics& diagnostics) {
    storage_ = &storage;
    diagnostics_ = &diagnostics;
    loadTimeZone();
}

void CardputerClock::loadTimeZone() {
    char stored[40] = {};
    if (storage_ != nullptr && storage_->loadPref(kTimeZoneKey, stored, sizeof(stored)) &&
        isTimeZoneId(stored)) {
        std::snprintf(tz_, sizeof(tz_), "%s", stored);
        return;
    }
    std::snprintf(tz_, sizeof(tz_), "%s", kDefaultTimeZoneId);
}

void CardputerClock::saveTimeZone() {
    if (storage_ != nullptr) {
        storage_->savePref(kTimeZoneKey, tz_, sizeof(tz_));
    }
}

void CardputerClock::setTimeZone(const char* id) {
    std::snprintf(tz_, sizeof(tz_), "%s", canonicalTimeZoneId(id));
    saveTimeZone();
}

const char* CardputerClock::timeZoneId() const { return tz_; }

void CardputerClock::synchronize() {
    configTime(0, 0, "pool.ntp.org");
    last_sync_ms_ = millis();
    if (diagnostics_ != nullptr) {
        diagnostics_->emit("CLOCK", "ntp start");
    }
}

void CardputerClock::update() {
    const time_t now = std::time(nullptr);
    if (now >= static_cast<time_t>(kMinValidUnix)) {
        last_unix_ = static_cast<int64_t>(now);
        synced_ = true;
    }
    if (synced_ && last_sync_ms_ != 0 && millis() - last_sync_ms_ >= kNtpRefreshMs) {
        synchronize();
    }
}

CivilTime CardputerClock::localTime() const {
    if (!synced_ || last_unix_ == 0) {
        return {};
    }
    return civilTimeAt(last_unix_, tz_);
}

}  // namespace luma
