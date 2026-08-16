#include "luma/platform/host/host-clock-adapter.h"

#include <chrono>
#include <ctime>

namespace luma {

uint32_t HostClockAdapter::millis() const {
    using namespace std::chrono;
    const auto now = steady_clock::now().time_since_epoch();
    return static_cast<uint32_t>(duration_cast<milliseconds>(now).count());
}

CivilTime HostClockAdapter::localTime() const {
    const std::time_t now = std::time(nullptr);
    const std::tm* local = std::localtime(&now);
    if (local == nullptr) {
        return {};
    }
    CivilTime time;
    time.hour = static_cast<uint8_t>(local->tm_hour);
    time.minute = static_cast<uint8_t>(local->tm_min);
    time.valid = true;
    return time;
}

}  // namespace luma
