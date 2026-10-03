#include "luma/platform/host/host-infrared.h"

#include <cstdio>

namespace luma {

HostInfrared::HostInfrared(Diagnostics& diagnostics) : diagnostics_(diagnostics) {}

bool HostInfrared::transmit(const Frame& frame) {
    char message[48] = {};
    std::snprintf(message, sizeof(message), "freq=%u pulses=%u",
                  static_cast<unsigned>(frame.frequency_khz), static_cast<unsigned>(frame.count));
    diagnostics_.emit("IR", message);
    return frame.count > 0 && frame.frequency_khz >= Infrared::kMinCarrierKhz &&
           frame.frequency_khz <= Infrared::kMaxCarrierKhz;
}

}  // namespace luma
