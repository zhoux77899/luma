#include "cardputer-battery-source.h"

#include <M5Cardputer.h>

#include "luma/core/battery-types.h"

namespace luma {

BatteryReading CardputerBatterySource::read() const {
    BatteryReading reading;
    const int voltage = M5.Power.getBatteryVoltage();
    if (voltage > 0 && voltage <= 65535) {
        const uint32_t sample = static_cast<uint32_t>(voltage);
        if (!have_smooth_) {
            smoothed_mv_ = sample;
            have_smooth_ = true;
        } else {
            smoothed_mv_ = (smoothed_mv_ * 7u + sample) / 8u;
        }
        reading.voltage_mv = static_cast<uint16_t>(smoothed_mv_);
        reading.voltage_valid = true;
        reading.percent = batteryPercentFromVoltageMv(reading.voltage_mv);
        reading.percent_valid = true;
    }
    return reading;
}

}  // namespace luma
