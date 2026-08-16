#include "luma/platform/host/host-clock-adapter.h"

#include <chrono>

namespace luma {

uint32_t HostClockAdapter::millis() const {
    using namespace std::chrono;
    const auto now = steady_clock::now().time_since_epoch();
    return static_cast<uint32_t>(duration_cast<milliseconds>(now).count());
}

}  // namespace luma
