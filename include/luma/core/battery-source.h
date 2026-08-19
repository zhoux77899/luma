#pragma once

#include "luma/core/battery-types.h"

namespace luma {

class BatterySource {
public:
    virtual ~BatterySource() = default;
    virtual BatteryReading read() const = 0;
};

}  // namespace luma
