#pragma once

#include "luma/core/battery-source.h"

namespace luma {

class HostBatterySource : public BatterySource {
public:
    BatteryReading reading;

    HostBatterySource() {
        reading.percent = 87;
        reading.voltage_mv = 4120;
        reading.charging = false;
        reading.percent_valid = true;
        reading.voltage_valid = true;
        reading.charging_valid = true;
    }

    BatteryReading read() const override { return reading; }
};

}  // namespace luma
