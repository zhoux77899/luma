#pragma once

#include "luma/core/audio.h"

namespace luma {

class Diagnostics;

class HostAudioAdapter : public Audio {
public:
    explicit HostAudioAdapter(Diagnostics& diagnostics);

    void play(const char* event) override;

private:
    Diagnostics& diagnostics_;
};

}  // namespace luma
