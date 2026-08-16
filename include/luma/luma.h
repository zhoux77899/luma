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
class Clock;
class Diagnostics;
class DisplaySurface;
class InputSource;
class Settings;
class Storage;

class Luma {
public:
    Luma();
    Luma(DisplaySurface& display, InputSource& input, Clock& clock, Storage& storage,
         Settings& settings, Diagnostics& diagnostics, Audio& audio);

    void begin();
    void update();

    bool registerApp(App& app);
    const char* currentAppId() const;
    AppManager& appManager();

private:
    void drawBootScreen();
    bool inputPresent(const InputFrame& frame) const;
    void finishBoot();
    void requestLauncherTimeRedraw();

    DisplaySurface& display_;
    InputSource& input_;
    Clock& clock_;
    Storage& storage_;
    Settings& settings_;
    Diagnostics& diagnostics_;
    Audio& audio_;
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
};

}  // namespace luma
