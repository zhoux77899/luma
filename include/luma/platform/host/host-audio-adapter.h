#pragma once

#include "luma/core/audio.h"

namespace luma {

class Diagnostics;

class HostAudioAdapter : public Audio {
public:
    explicit HostAudioAdapter(Diagnostics& diagnostics);

    void setVolume(uint8_t percent) override;
    void play(const char* event) override;

private:
    Diagnostics& diagnostics_;
    uint8_t volume_ = 80;
};

}  // namespace luma
