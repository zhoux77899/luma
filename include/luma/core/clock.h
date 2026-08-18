#pragma once

#include <cstdint>

namespace luma {

class Diagnostics;
class Storage;

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
    virtual void update() {}
    virtual void synchronize() {}
    virtual void attach(Storage& storage, Diagnostics& diagnostics) {
        (void)storage;
        (void)diagnostics;
    }
    virtual void setTimeZone(const char* id) { (void)id; }
    virtual const char* timeZoneId() const { return "UTC"; }
};

}  // namespace luma
