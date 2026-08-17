#include "cardputer-audio-adapter.h"

#include <M5Cardputer.h>

namespace luma {
namespace {

constexpr uint32_t kClickHz = 2000;
constexpr uint32_t kClickMs = 40;

}  // namespace

void CardputerAudioAdapter::setVolume(uint8_t percent) {
    const uint8_t clamped = percent > 100 ? 100 : percent;
    M5Cardputer.Speaker.setVolume(static_cast<uint8_t>((clamped * 255) / 100));
}

void CardputerAudioAdapter::play(const char* event) {
    (void)event;
    M5Cardputer.Speaker.tone(kClickHz, kClickMs);
}

}  // namespace luma
