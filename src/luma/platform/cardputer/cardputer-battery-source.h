#pragma once

#include <cstdint>

#include "luma/core/battery-source.h"

namespace luma {

class CardputerBatterySource : public BatterySource {
public:
    BatteryReading read() const override;

private:
    mutable uint32_t smoothed_mv_ = 0;
    mutable bool have_smooth_ = false;
};

}  // namespace luma
