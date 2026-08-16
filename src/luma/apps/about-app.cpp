#include "luma/apps/about-app.h"

#include "luma/apps/placeholder-page.h"
#include "luma/core/app-context.h"

namespace luma {

const char* AboutApp::id() const { return "about"; }
const char* AboutApp::name() const { return "About"; }

void AboutApp::onEnter(AppContext& context) { context_ = &context; }

void AboutApp::update(const InputFrame& input) { (void)input; }

void AboutApp::draw() {
    if (context_ != nullptr) {
        drawPlaceholderPage(*context_, name());
    }
}

}  // namespace luma
