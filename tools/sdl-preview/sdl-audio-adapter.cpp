#include "sdl-audio-adapter.h"

#include <SDL.h>

#include <cstdint>

namespace luma {
namespace {

constexpr int kSampleRate = 22050;
constexpr int kClickHz = 2000;
constexpr int kClickMs = 40;

}  // namespace

SdlAudioAdapter::~SdlAudioAdapter() {
    if (device_ != 0) {
        SDL_CloseAudioDevice(device_);
        device_ = 0;
    }
}

void SdlAudioAdapter::begin() {
    SDL_AudioSpec want{};
    want.freq = kSampleRate;
    want.format = AUDIO_S16SYS;
    want.channels = 1;
    want.samples = 256;
    device_ = SDL_OpenAudioDevice(nullptr, 0, &want, nullptr, 0);
    if (device_ != 0) {
        SDL_PauseAudioDevice(device_, 0);
    }
}

void SdlAudioAdapter::setVolume(uint8_t percent) { volume_ = percent > 100 ? 100 : percent; }

void SdlAudioAdapter::play(const char* event) {
    (void)event;
    if (device_ == 0 || volume_ == 0) {
        return;
    }

    const int samples = kSampleRate * kClickMs / 1000;
    const int half_period = kSampleRate / (kClickHz * 2);
    if (samples <= 0 || half_period <= 0) {
        return;
    }

    const int16_t amplitude = static_cast<int16_t>((30000 * static_cast<int>(volume_)) / 100);
    int16_t buffer[kSampleRate * kClickMs / 1000] = {};
    for (int i = 0; i < samples; ++i) {
        buffer[i] = ((i / half_period) % 2) == 0 ? amplitude : static_cast<int16_t>(-amplitude);
    }
    SDL_QueueAudio(device_, buffer, static_cast<uint32_t>(samples) * sizeof(int16_t));
}

}  // namespace luma
