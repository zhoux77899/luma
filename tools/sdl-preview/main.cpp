#include "sdl-display-adapter.h"
#include "sdl-input-adapter.h"

#include "luma/core/file-storage.h"
#include "luma/core/settings.h"
#include "luma/luma.h"
#include "luma/platform/host/host-audio-adapter.h"
#include "luma/platform/host/host-clock-adapter.h"
#include "luma/platform/host/host-diagnostics.h"

#include <SDL.h>

int main(int, char**) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        return 1;
    }
    SDL_StartTextInput();

    luma::HostDiagnostics diagnostics;
    luma::SdlDisplayAdapter display;
    luma::SdlInputAdapter input;
    luma::HostClockAdapter clock;
    luma::HostStorageAdapter storage("data");
    luma::Settings settings;
    luma::HostAudioAdapter audio(diagnostics);
    luma::Luma luma(display, input, clock, storage, settings, diagnostics, audio);

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
