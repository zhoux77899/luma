#pragma once

#include "luma/core/clock.h"

#include <cstdint>

namespace luma {

class HostClockAdapter : public Clock {
public:
    uint32_t millis() const override;
    CivilTime localTime() const override;
    void update() override;
    void synchronize() override;
    void attach(Storage& storage, Diagnostics& diagnostics) override;
    void setTimeZone(const char* id) override;
    const char* timeZoneId() const override;

private:
    void loadTimeZone();
    void saveTimeZone();
    int64_t unixNow() const;
    int32_t hostOffsetEastSec() const;

    Storage* storage_ = nullptr;
    Diagnostics* diagnostics_ = nullptr;
    char tz_[40] = "UTC";
    bool tz_ready_ = false;
};

}  // namespace luma
