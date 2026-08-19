#pragma once

#include <cstdint>

namespace luma {

struct BatteryReading {
    uint8_t percent = 0;
    uint16_t voltage_mv = 0;
    bool charging = false;
    bool percent_valid = false;
    bool voltage_valid = false;
    bool charging_valid = false;
};

struct BatterySample {
    BatteryReading reading;
    uint32_t millis = 0;
    uint32_t unix_utc = 0;
    uint8_t run_id = 0;
};

inline uint8_t batteryFillLevel(const BatteryReading& reading) {
    if (!reading.percent_valid || reading.percent == 0) {
        return 0;
    }
    if (reading.percent <= 20) {
        return 1;
    }
    if (reading.percent <= 40) {
        return 2;
    }
    if (reading.percent <= 60) {
        return 3;
    }
    if (reading.percent <= 80) {
        return 4;
    }
    return 5;
}

inline bool batteryHistoryGap(const BatterySample& previous, const BatterySample& next) {
    if (previous.run_id != next.run_id) {
        return true;
    }
    return (next.millis - previous.millis) > 90000;
}

}  // namespace luma
