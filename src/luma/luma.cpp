#include "luma/luma.h"

#include "luma/assets/luma-logo-boot.h"
#include "luma/core/audio.h"
#include "luma/core/clock.h"
#include "luma/core/diagnostics.h"
#include "luma/core/display.h"
#include "luma/core/input-source.h"
#include "luma/core/settings.h"
#include "luma/core/storage.h"
#include "luma/ui/theme.h"

#include <cstring>

namespace luma {
namespace {

constexpr uint32_t kBootDurationMs = 1000;

}  // namespace

Luma::Luma(DisplaySurface& display, InputSource& input, Clock& clock, Storage& storage,
           Settings& settings, Diagnostics& diagnostics, Audio& audio)
    : display_(display),
      input_(input),
      clock_(clock),
      storage_(storage),
      settings_(settings),
      diagnostics_(diagnostics),
      audio_(audio),
      context_(display, settings, storage, clock),
      input_manager_(input, diagnostics),
      app_manager_(context_, diagnostics),
      launcher_(app_manager_) {}

void Luma::begin() {
    display_.begin();
    audio_.begin();
    storage_.begin();
    settings_.load();
    registerApp(launcher_);
    registerApp(settings_app_);
    registerApp(about_app_);
    registerApp(notes_app_);
    drawBootScreen();
    diagnostics_.emit("BOOT", "Luma Cardputer ADV started");
    booting_ = true;
    boot_started_ms_ = clock_.millis();
}

void Luma::update() {
    InputFrame frame;
    input_manager_.poll(frame);

    if (booting_) {
        const bool timed_out = (clock_.millis() - boot_started_ms_) >= kBootDurationMs;
        if (timed_out || inputPresent(frame)) {
            finishBoot();
        }
        settings_.processDeferredSaves();
        storage_.processDeferredSaves();
        return;
    }

    app_manager_.dispatch(frame);
    requestLauncherTimeRedraw();
    app_manager_.drawIfNeeded();
    settings_.processDeferredSaves();
    storage_.processDeferredSaves();
}

bool Luma::registerApp(App& app) {
    return app_manager_.registerApp({&app, app.id(), app.name(), app.shortcut()});
}

const char* Luma::currentAppId() const { return app_manager_.currentId(); }

AppManager& Luma::appManager() { return app_manager_; }

void Luma::drawBootScreen() {
    display_.beginFrame();
    display_.clear(theme::kBootCanvas);
    const int x = (display_.width() - assets::kLogoBootSize) / 2;
    const int y = (display_.height() - assets::kLogoBootSize) / 2;
    display_.drawBitmap({x, y}, assets::kLogoBootSize, assets::kLogoBootSize, assets::kLogoBoot);
    display_.endFrame();
}

bool Luma::inputPresent(const InputFrame& frame) const {
    return frame.action != InputAction::None || frame.textLength > 0;
}

void Luma::finishBoot() {
    booting_ = false;
    const CivilTime time = clock_.localTime();
    last_header_minute_ = time.valid ? time.minute : 255;
    app_manager_.enter(AppManager::kLauncherId);
    app_manager_.drawIfNeeded();
}

void Luma::requestLauncherTimeRedraw() {
    if (std::strcmp(app_manager_.currentId(), AppManager::kLauncherId) != 0) {
        return;
    }
    const CivilTime time = clock_.localTime();
    const uint8_t minute_key = time.valid ? time.minute : 255;
    if (minute_key == last_header_minute_) {
        return;
    }
    last_header_minute_ = minute_key;
    context_.requestRedraw();
}

}  // namespace luma
