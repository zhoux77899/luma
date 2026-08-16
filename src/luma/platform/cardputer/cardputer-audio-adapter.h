#pragma once

#include "luma/core/audio.h"

namespace luma {

class CardputerAudioAdapter : public Audio {
public:
    void play(const char* event) override;
};

}  // namespace luma
