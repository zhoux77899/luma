#include "luma/apps/launcher-app.h"

#include "luma/core/app-context.h"
#include "luma/core/display.h"

namespace luma {
namespace {

constexpr Color kBlack{0, 0, 0};
constexpr Color kWhite{255, 255, 255};

}  // namespace

const char* LauncherApp::id() const { return "launcher"; }
const char* LauncherApp::name() const { return "Launcher"; }

void LauncherApp::onEnter(AppContext& context) { context_ = &context; }

void LauncherApp::update(const InputFrame& input) { (void)input; }

void LauncherApp::draw() {
    if (context_ == nullptr) {
        return;
    }
    DisplaySurface& display = context_->display();
    display.beginFrame();
    display.clear(kBlack);
    display.drawText({10, 10}, {kWhite, 1}, "Launcher");
    display.endFrame();
}

}  // namespace luma
