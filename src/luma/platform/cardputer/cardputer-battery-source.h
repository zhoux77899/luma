#pragma once

#include "luma/core/battery-source.h"

namespace luma {

class CardputerBatterySource : public BatterySource {
public:
    BatteryReading read() const override;
};

}  // namespace luma
