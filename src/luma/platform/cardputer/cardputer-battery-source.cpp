#include "cardputer-battery-source.h"

#include <M5Cardputer.h>

namespace luma {

BatteryReading CardputerBatterySource::read() const {
    BatteryReading reading;
    const int level = M5.Power.getBatteryLevel();
    if (level >= 0 && level <= 100) {
        reading.percent = static_cast<uint8_t>(level);
        reading.percent_valid = true;
    }
    const int voltage = M5.Power.getBatteryVoltage();
    if (voltage > 0 && voltage <= 65535) {
        reading.voltage_mv = static_cast<uint16_t>(voltage);
        reading.voltage_valid = true;
    }
    reading.charging = M5.Power.isCharging();
    reading.charging_valid = true;
    return reading;
}

}  // namespace luma
