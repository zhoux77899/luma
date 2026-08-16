#include "luma/apps/settings-app.h"

#include "luma/apps/placeholder-page.h"
#include "luma/core/app-context.h"

namespace luma {

const char* SettingsApp::id() const { return "settings"; }
const char* SettingsApp::name() const { return "Settings"; }

void SettingsApp::onEnter(AppContext& context) { context_ = &context; }

void SettingsApp::update(const InputFrame& input) { (void)input; }

void SettingsApp::draw() {
    if (context_ != nullptr) {
        drawPlaceholderPage(*context_, name());
    }
}

}  // namespace luma
