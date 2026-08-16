#pragma once

#include <cstdint>

namespace luma {

class Audio {
public:
    virtual ~Audio() = default;
    virtual void begin() {}
    virtual void setVolume(uint8_t percent) { (void)percent; }
    virtual void play(const char* event) = 0;
};

}  // namespace luma
