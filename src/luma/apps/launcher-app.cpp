#include "luma/apps/launcher-app.h"

#include "luma/assets/luma-logo-header.h"
#include "luma/core/app-context.h"
#include "luma/core/app-manager.h"
#include "luma/core/display.h"
#include "luma/core/settings.h"
#include "luma/ui/components.h"
#include "luma/ui/renderer.h"
#include "luma/ui/theme.h"

#include <cstring>

namespace luma {

LauncherApp::LauncherApp(AppManager& manager) : manager_(manager) {}

const char* LauncherApp::id() const { return AppManager::kLauncherId; }
const char* LauncherApp::name() const { return "LAUNCHER"; }

void LauncherApp::onEnter(AppContext& context) {
    context_ = &context;
    selected_ = 0;
}

int LauncherApp::launchableCount() const {
    int count = 0;
    for (size_t i = 0; i < manager_.appCount(); ++i) {
        if (std::strcmp(manager_.appAt(i).id, AppManager::kLauncherId) != 0) {
            ++count;
        }
    }
    return count;
}

void LauncherApp::collectLaunchable(const char** ids, const char** names) const {
    int count = 0;
    for (size_t i = 0; i < manager_.appCount(); ++i) {
        const AppDescriptor& descriptor = manager_.appAt(i);
        if (std::strcmp(descriptor.id, AppManager::kLauncherId) == 0) {
            continue;
        }
        ids[count] = descriptor.id;
        names[count] = descriptor.name;
        ++count;
    }
}

void LauncherApp::moveSelection(InputAction action, int count) {
    if (count <= 0) {
        return;
    }
    const int column = selected_ % 2;
    const int row = selected_ / 2;
    int next = selected_;
    if (action == InputAction::Right && column == 0 && selected_ + 1 < count) {
        next = selected_ + 1;
    } else if (action == InputAction::Left && column == 1) {
        next = selected_ - 1;
    } else if (action == InputAction::Down && selected_ + 2 < count) {
        next = selected_ + 2;
    } else if (action == InputAction::Up && row > 0) {
        next = selected_ - 2;
    }
    if (next != selected_) {
        selected_ = next;
        if (context_ != nullptr) {
            context_->requestRedraw();
        }
    }
}

void LauncherApp::update(const InputFrame& input) {
    if (context_ == nullptr) {
        return;
    }
    const int count = launchableCount();
    if (count <= 0) {
        return;
    }
    if (selected_ >= count) {
        selected_ = count - 1;
    }

    if (input.action == InputAction::Confirm) {
        const char* ids[kMaxLaunchable] = {};
        const char* names[kMaxLaunchable] = {};
        collectLaunchable(ids, names);
        context_->requestEnter(ids[selected_]);
        return;
    }

    moveSelection(input.action, count);
}

void LauncherApp::draw() {
    if (context_ == nullptr) {
        return;
    }

    const char* ids[kMaxLaunchable] = {};
    const char* names[kMaxLaunchable] = {};
    collectLaunchable(ids, names);
    const int count = launchableCount();

    char time_label[8] = {};
    formatCivilTime(context_->clock().localTime(), time_label, sizeof(time_label));

    const theme::Palette palette = theme::paletteFor(context_->settings().theme());
    UiRenderer renderer(context_->display(), palette);
    renderer.beginFrame();
    renderer.clearAppCanvas();
    drawAppHeader(renderer.surface(), palette, assets::kLogoHeader, "LUMA", time_label);
    for (int i = 0; i < count; ++i) {
        drawAppCard(renderer.surface(), palette, i % 2, i / 2, names[i], theme::appCardColor(i),
                    i == selected_);
    }
    renderer.endFrame();
}

}  // namespace luma
