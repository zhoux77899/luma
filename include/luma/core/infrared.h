#pragma once

#include <cstdint>

namespace luma {

class Infrared {
public:
    static constexpr uint16_t kMaxDurations = 160;
    static constexpr uint16_t kMinCarrierKhz = 30;
    static constexpr uint16_t kMaxCarrierKhz = 60;

    struct Frame {
        uint16_t frequency_khz = 38;
        uint16_t count = 0;
        uint16_t durations[kMaxDurations] = {};
    };

    virtual ~Infrared() = default;
    virtual void begin() {}
    virtual bool transmit(const Frame& frame) = 0;
};

}  // namespace luma
