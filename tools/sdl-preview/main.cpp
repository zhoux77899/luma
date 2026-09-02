#include "preview-actions.h"
#include "preview-host.h"
#include "sdl-audio-adapter.h"
#include "sdl-display-adapter.h"
#include "sdl-input-adapter.h"

#include "luma/core/battery.h"
#include "luma/core/file-storage.h"
#include "luma/core/network.h"
#include "luma/core/settings.h"
#include "luma/luma.h"
#include "luma/platform/host/host-battery-source.h"
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
    luma::PreviewHost host;
    luma::HostClockAdapter clock;
    luma::HostStorageAdapter storage("data");
    luma::Settings settings;
    luma::SdlAudioAdapter audio;
    luma::HostWifiRadio radio;
    luma::Network network;
    luma::HostBatterySource battery_source;
    luma::Battery battery;
    network.attach(radio, storage, diagnostics, clock);
    battery.attach(battery_source, storage, diagnostics, clock);
    luma::Luma luma(display, input, clock, storage, settings, diagnostics, audio, network, battery);

    luma.begin();
    host.attach(display.window());
    input.setHost(&host);

    while (!input.quitRequested()) {
        luma.update();
        host.pump();
        luma::PreviewCommand command = input.takeCommand();
        if (command == luma::PreviewCommand::None) {
            command = host.takeCommand();
        }
        luma::runPreviewCommand(command, display);
        display.endFrame();
        SDL_Delay(16);
    }

    SDL_StopTextInput();
    SDL_Quit();
    return 0;
}
