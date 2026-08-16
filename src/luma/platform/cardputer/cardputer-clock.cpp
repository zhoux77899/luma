#include "cardputer-clock.h"

#include <Arduino.h>

#include <ctime>

namespace luma {

uint32_t CardputerClock::millis() const { return ::millis(); }

CivilTime CardputerClock::localTime() const {
    const std::time_t now = std::time(nullptr);
    const std::tm* local = std::localtime(&now);
    if (local == nullptr || local->tm_year + 1900 < 2024) {
        return {};
    }
    CivilTime time;
    time.hour = static_cast<uint8_t>(local->tm_hour);
    time.minute = static_cast<uint8_t>(local->tm_min);
    time.valid = true;
    return time;
}

}  // namespace luma
