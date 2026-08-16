#pragma once

#include "luma/core/clock.h"

namespace luma {

class CardputerClock : public Clock {
public:
    uint32_t millis() const override;
};

}  // namespace luma
