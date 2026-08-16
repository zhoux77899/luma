#include "cardputer-clock.h"

#include <Arduino.h>

namespace luma {

uint32_t CardputerClock::millis() const { return ::millis(); }

}  // namespace luma
