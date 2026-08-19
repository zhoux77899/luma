#include "luma/luma.h"

#include "luma/assets/luma-logo-boot.h"
#include "luma/core/audio.h"
#include "luma/core/clock.h"
#include "luma/core/diagnostics.h"
#include "luma/core/display.h"
#include "luma/core/input-source.h"
#include "luma/core/network.h"
#include "luma/core/settings.h"
#include "luma/core/storage.h"
#include "luma/ui/theme.h"

namespace luma {
namespace {

constexpr uint32_t kBootDurationMs = 1000;

}  // namespace

Luma::Luma(DisplaySurface& display, InputSource& input, Clock& clock, Storage& storage,
           Settings& settings, Diagnostics& diagnostics, Audio& audio, Network& network)
    : display_(display),
      input_(input),
      clock_(clock),
      storage_(storage),
      settings_(settings),
      diagnostics_(diagnostics),
      audio_(audio),
      network_(network),
      context_(display, settings, storage, clock, diagnostics, network),
      input_manager_(input, diagnostics),
      app_manager_(context_, diagnostics),
      launcher_(app_manager_) {}

void Luma::begin() {
    display_.begin();
    audio_.begin();
    storage_.begin();
    settings_.attach(storage_, diagnostics_, clock_);
    settings_.load();
    clock_.attach(storage_, diagnostics_);
    network_.load();
    network_.begin();
    display_.setBrightness(settings_.brightness());
    audio_.setVolume(settings_.volume());
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

    network_.update();
    if (network_.takeConnectedEdge()) {
        clock_.synchronize();
    }
    clock_.update();

    if (booting_) {
        const bool timed_out = (clock_.millis() - boot_started_ms_) >= kBootDurationMs;
        if (timed_out || inputPresent(frame)) {
            finishBoot();
        }
        settings_.processDeferredSaves(clock_.millis());
        storage_.processDeferredSaves();
        return;
    }

    if (frame.action == InputAction::Confirm) {
        playUiSound("click");
    }
    app_manager_.dispatch(frame);
    if (context_.takeUiSound()) {
        playUiSound("click");
    }
    requestHeaderRedraw();
    app_manager_.drawIfNeeded();
    settings_.processDeferredSaves(clock_.millis());
    storage_.processDeferredSaves();
}

bool Luma::registerApp(App& app) {
    return app_manager_.registerApp({&app, app.id(), app.name(), app.shortcut()});
}

const char* Luma::currentAppId() const { return app_manager_.currentId(); }

AppManager& Luma::appManager() { return app_manager_; }

void Luma::drawBootScreen() {
    const theme::Palette palette = theme::paletteFor(settings_.theme());
    display_.beginFrame();
    display_.clear(palette.boot_canvas);
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
    last_header_network_ = headerNetworkKey();
    last_scan_key_ = static_cast<uint8_t>(
        (network_.scanInProgress() ? 0x80 : 0) | (network_.publicScanCount() & 0x7F));
    app_manager_.enter(AppManager::kLauncherId);
    app_manager_.drawIfNeeded();
}

uint8_t Luma::headerNetworkKey() const {
    return static_cast<uint8_t>(
        (static_cast<unsigned>(network_.state()) << 4) |
        static_cast<unsigned>(network_.signalStrength()));
}

void Luma::requestHeaderRedraw() {
    const CivilTime time = clock_.localTime();
    const uint8_t minute_key = time.valid ? time.minute : 255;
    const uint8_t network_key = headerNetworkKey();
    const uint8_t scan_key = static_cast<uint8_t>(
        (network_.scanInProgress() ? 0x80 : 0) | (network_.publicScanCount() & 0x7F));
    if (minute_key == last_header_minute_ && network_key == last_header_network_ &&
        scan_key == last_scan_key_) {
        return;
    }
    last_header_minute_ = minute_key;
    last_header_network_ = network_key;
    last_scan_key_ = scan_key;
    context_.requestRedraw();
}

void Luma::playUiSound(const char* event) {
    audio_.setVolume(settings_.volume());
    if (settings_.volume() > 0) {
        audio_.play(event);
    }
}

}  // namespace luma
