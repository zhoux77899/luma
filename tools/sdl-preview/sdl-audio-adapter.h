#pragma once

#include "luma/core/audio.h"

#include <cstdint>

namespace luma {

class SdlAudioAdapter : public Audio {
public:
    ~SdlAudioAdapter() override;

    void begin() override;
    void setVolume(uint8_t percent) override;
    void play(const char* event) override;

private:
    uint32_t device_ = 0;
    uint8_t volume_ = 80;
};

}  // namespace luma
