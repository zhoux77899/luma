#pragma once

#include <cstdint>

namespace luma {

class Clock {
public:
    virtual ~Clock() = default;
    virtual uint32_t millis() const = 0;
};

}  // namespace luma
