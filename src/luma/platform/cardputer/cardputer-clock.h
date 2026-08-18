#pragma once

#include "luma/core/clock.h"

#include <cstdint>

namespace luma {

class CardputerClock : public Clock {
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

    Storage* storage_ = nullptr;
    Diagnostics* diagnostics_ = nullptr;
    char tz_[40] = "UTC";
    bool synced_ = false;
    int64_t last_unix_ = 0;
    uint32_t last_sync_ms_ = 0;
};

}  // namespace luma
