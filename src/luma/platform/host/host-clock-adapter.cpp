#include "luma/platform/host/host-clock-adapter.h"

#include "luma/core/diagnostics.h"
#include "luma/core/storage.h"
#include "luma/core/time-zone.h"

#include <chrono>
#include <cstdio>
#include <cstring>
#include <ctime>

#ifdef _WIN32
#include <windows.h>
#endif

namespace luma {
namespace {

constexpr const char* kTimeZoneKey = "timezone";

}  // namespace

uint32_t HostClockAdapter::millis() const {
    using namespace std::chrono;
    const auto now = steady_clock::now().time_since_epoch();
    return static_cast<uint32_t>(duration_cast<milliseconds>(now).count());
}

int64_t HostClockAdapter::unixNow() const { return static_cast<int64_t>(std::time(nullptr)); }

int32_t HostClockAdapter::hostOffsetEastSec() const {
#ifdef _WIN32
    TIME_ZONE_INFORMATION info;
    const DWORD mode = GetTimeZoneInformation(&info);
    LONG bias = info.Bias;
    if (mode == TIME_ZONE_ID_DAYLIGHT) {
        bias += info.DaylightBias;
    } else {
        bias += info.StandardBias;
    }
    return static_cast<int32_t>(-bias * 60);
#else
    const std::time_t now = std::time(nullptr);
    std::tm local{};
    localtime_r(&now, &local);
    return static_cast<int32_t>(local.tm_gmtoff);
#endif
}

void HostClockAdapter::loadTimeZone() {
    char stored[40] = {};
    if (storage_ != nullptr && storage_->loadPref(kTimeZoneKey, stored, sizeof(stored)) &&
        isTimeZoneId(stored)) {
        std::snprintf(tz_, sizeof(tz_), "%s", stored);
        tz_ready_ = true;
        return;
    }
    std::snprintf(tz_, sizeof(tz_), "%s", matchTimeZoneId(hostOffsetEastSec(), unixNow()));
    tz_ready_ = true;
}

void HostClockAdapter::saveTimeZone() {
    if (storage_ != nullptr) {
        storage_->savePref(kTimeZoneKey, tz_, sizeof(tz_));
    }
}

void HostClockAdapter::attach(Storage& storage, Diagnostics& diagnostics) {
    storage_ = &storage;
    diagnostics_ = &diagnostics;
    loadTimeZone();
}

void HostClockAdapter::setTimeZone(const char* id) {
    std::snprintf(tz_, sizeof(tz_), "%s", canonicalTimeZoneId(id));
    tz_ready_ = true;
    saveTimeZone();
}

const char* HostClockAdapter::timeZoneId() const { return tz_; }

void HostClockAdapter::synchronize() {
    if (diagnostics_ != nullptr) {
        diagnostics_->emit("CLOCK", "host time");
    }
}

void HostClockAdapter::update() {
    if (!tz_ready_) {
        loadTimeZone();
    }
}

CivilTime HostClockAdapter::localTime() const {
    return civilTimeAt(unixNow(), tz_ready_ ? tz_ : matchTimeZoneId(hostOffsetEastSec(), unixNow()));
}

}  // namespace luma
