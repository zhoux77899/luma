#pragma once

#include <cstdint>

#include "luma/apps/about-app.h"
#include "luma/apps/launcher-app.h"
#include "luma/apps/notes-app.h"
#include "luma/apps/settings-app.h"
#include "luma/core/app-context.h"
#include "luma/core/app-manager.h"
#include "luma/core/input-manager.h"

namespace luma {

class Audio;
class Battery;
class Clock;
class Diagnostics;
class DisplaySurface;
class InputSource;
class Network;
class Settings;
class Storage;

class Luma {
public:
    Luma();
    Luma(DisplaySurface& display, InputSource& input, Clock& clock, Storage& storage,
         Settings& settings, Diagnostics& diagnostics, Audio& audio, Network& network,
         Battery& battery);

    void begin();
    void update();

    bool registerApp(App& app);
    const char* currentAppId() const;
    AppManager& appManager();

private:
    void drawBootScreen();
    bool inputPresent(const InputFrame& frame) const;
    void finishBoot();
    void requestHeaderRedraw();
    void playUiSound(const char* event);
    uint8_t headerNetworkKey() const;
    uint8_t headerBatteryKey() const;

    DisplaySurface& display_;
    InputSource& input_;
    Clock& clock_;
    Storage& storage_;
    Settings& settings_;
    Diagnostics& diagnostics_;
    Audio& audio_;
    Network& network_;
    Battery& battery_;
    AppContext context_;
    InputManager input_manager_;
    AppManager app_manager_;
    LauncherApp launcher_;
    SettingsApp settings_app_;
    AboutApp about_app_;
    NotesApp notes_app_;
    bool booting_ = false;
    uint32_t boot_started_ms_ = 0;
    uint8_t last_header_minute_ = 254;
    uint8_t last_header_network_ = 255;
    uint8_t last_header_battery_ = 255;
    uint8_t last_scan_key_ = 255;
};

}  // namespace luma
