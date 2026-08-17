#include "luma/platform/host/host-audio-adapter.h"

#include "luma/core/diagnostics.h"

namespace luma {

HostAudioAdapter::HostAudioAdapter(Diagnostics& diagnostics) : diagnostics_(diagnostics) {}

void HostAudioAdapter::setVolume(uint8_t percent) { volume_ = percent; }

void HostAudioAdapter::play(const char* event) { diagnostics_.emit("AUDIO", event); }

}  // namespace luma
