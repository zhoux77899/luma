#pragma once

#include <cstdint>

namespace luma {

struct CivilTime {
    uint8_t hour = 0;
    uint8_t minute = 0;
    bool valid = false;
};

class Clock {
public:
    virtual ~Clock() = default;
    virtual uint32_t millis() const = 0;
    virtual CivilTime localTime() const = 0;
};

}  // namespace luma
