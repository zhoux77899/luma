#include "sdl-audio-adapter.h"
#include "sdl-display-adapter.h"
#include "sdl-input-adapter.h"

#include "luma/core/file-storage.h"
#include "luma/core/network.h"
#include "luma/core/settings.h"
#include "luma/luma.h"
#include "luma/platform/host/host-clock-adapter.h"
#include "luma/platform/host/host-diagnostics.h"
#include "luma/platform/host/host-wifi-radio.h"

#include <SDL.h>

int main(int, char**) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        return 1;
    }
    SDL_StartTextInput();

    luma::HostDiagnostics diagnostics;
    luma::SdlDisplayAdapter display;
    luma::SdlInputAdapter input;
    luma::HostClockAdapter clock;
    luma::HostStorageAdapter storage("data");
    luma::Settings settings;
    luma::SdlAudioAdapter audio;
    luma::HostWifiRadio radio;
    luma::Network network;
    network.attach(radio, storage, diagnostics, clock);
    luma::Luma luma(display, input, clock, storage, settings, diagnostics, audio, network);

    luma.begin();

    while (!input.quitRequested()) {
        luma.update();
        display.endFrame();
        SDL_Delay(16);
    }

    SDL_StopTextInput();
    SDL_Quit();
    return 0;
}
