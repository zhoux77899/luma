#include "luma/luma.h"

#include "luma/core/clock.h"
#include "luma/core/diagnostics.h"
#include "luma/core/display.h"
#include "luma/core/input-source.h"
#include "luma/core/settings.h"
#include "luma/core/storage.h"

namespace luma {
namespace {

constexpr Color kBlack{0, 0, 0};
constexpr Color kWhite{255, 255, 255};

}  // namespace

Luma::Luma(DisplaySurface& display, InputSource& input, Clock& clock, Storage& storage,
           Settings& settings, Diagnostics& diagnostics)
    : display_(display),
      input_(input),
      clock_(clock),
      storage_(storage),
      settings_(settings),
      diagnostics_(diagnostics),
      context_(display, settings, storage, clock),
      input_manager_(input, diagnostics),
      app_manager_(context_, diagnostics) {}

void Luma::begin() {
    display_.begin();
    storage_.begin();
    settings_.load();
    registerApp(launcher_);
    drawBootScreen();
    diagnostics_.emit("BOOT", "Luma Cardputer ADV started");
    app_manager_.enter(AppManager::kLauncherId);
    app_manager_.drawIfNeeded();
}

void Luma::update() {
    InputFrame frame;
    input_manager_.poll(frame);
    app_manager_.dispatch(frame);
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
    display_.clear(kBlack);
    display_.drawText({10, 10}, {kWhite, 2}, "Luma / Cardputer ADV");
    display_.endFrame();
}

}  // namespace luma
