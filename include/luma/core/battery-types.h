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

inline uint8_t batteryPercentFromVoltageMv(uint16_t mv) {
    constexpr int kEmptyMv = 3300;
    constexpr int kSpanMv = 4150 - 3350;
    if (static_cast<int>(mv) <= kEmptyMv) {
        return 0;
    }
    const int level = (static_cast<int>(mv) - kEmptyMv) * 100 / kSpanMv;
    if (level >= 100) {
        return 100;
    }
    return static_cast<uint8_t>(level);
}

enum class BatteryBand : uint8_t {
    Critical = 0,
    Warn = 1,
    Ok = 2,
};

inline BatteryBand batteryBand(uint8_t percent) {
    if (percent <= 20) {
        return BatteryBand::Critical;
    }
    if (percent <= 30) {
        return BatteryBand::Warn;
    }
    return BatteryBand::Ok;
}

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
