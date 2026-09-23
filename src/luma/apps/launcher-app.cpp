#include "luma/apps/launcher-app.h"

#include "luma/assets/luma-logo-header.h"
#include "luma/core/app-context.h"
#include "luma/core/app-manager.h"
#include "luma/core/battery.h"
#include "luma/core/display.h"
#include "luma/core/network.h"
#include "luma/core/settings.h"
#include "luma/ui/components.h"
#include "luma/ui/layout.h"
#include "luma/ui/renderer.h"
#include "luma/ui/theme.h"

#include <algorithm>
#include <cstring>

namespace luma {
namespace {

constexpr int kCardColumns = 2;
constexpr int kCardsPerPage = kCardColumns * layout::kCardRows;

}  // namespace

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

const AppDescriptor* LauncherApp::launchableAt(int index) const {
    for (size_t i = 0; i < manager_.appCount(); ++i) {
        const AppDescriptor& descriptor = manager_.appAt(i);
        if (std::strcmp(descriptor.id, AppManager::kLauncherId) == 0) {
            continue;
        }
        if (index-- == 0) {
            return &descriptor;
        }
    }
    return nullptr;
}

void LauncherApp::moveSelection(InputAction action, int count) {
    if (count <= 0) {
        return;
    }
    const int page = selected_ / kCardsPerPage;
    const int slot = selected_ % kCardsPerPage;
    const int column = slot % kCardColumns;
    const int row = slot / kCardColumns;
    int page_start = page * kCardsPerPage;
    int page_size = std::min(kCardsPerPage, count - page_start);
    int next_slot = slot;
    int page_step = 0;
    switch (action) {
        case InputAction::Right:
            if (column + 1 < kCardColumns && slot + 1 < page_size) {
                next_slot = slot + 1;
            } else {
                page_step = 1;
            }
            break;
        case InputAction::Left:
            if (column > 0) {
                next_slot = slot - 1;
            } else {
                page_step = -1;
            }
            break;
        case InputAction::Down:
            if (slot + kCardColumns < page_size) {
                next_slot = slot + kCardColumns;
            } else {
                page_step = 1;
            }
            break;
        case InputAction::Up:
            if (row > 0) {
                next_slot = slot - kCardColumns;
            } else {
                page_step = -1;
            }
            break;
        default:
            return;
    }

    if (page_step != 0) {
        if (count <= kCardsPerPage) {
            return;
        }
        const int page_count = (count + kCardsPerPage - 1) / kCardsPerPage;
        const int next_page = (page + page_step + page_count) % page_count;
        page_start = next_page * kCardsPerPage;
        page_size = std::min(kCardsPerPage, count - page_start);
        if (action == InputAction::Left || action == InputAction::Right) {
            const int target_row = std::min(row, (page_size - 1) / kCardColumns);
            next_slot = target_row * kCardColumns;
            if (action == InputAction::Left) {
                next_slot = std::min(next_slot + kCardColumns - 1, page_size - 1);
            }
        } else {
            const int target_column = std::min(column, page_size - 1);
            next_slot = target_column;
            if (action == InputAction::Up) {
                next_slot += ((page_size - 1 - target_column) / kCardColumns) * kCardColumns;
            }
        }
    }
    const int next = page_start + next_slot;
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
        if (const AppDescriptor* descriptor = launchableAt(selected_)) {
            context_->requestEnter(descriptor->id);
        }
        return;
    }

    moveSelection(input.action, count);
}

void LauncherApp::draw() {
    if (context_ == nullptr) {
        return;
    }

    const int count = launchableCount();
    const int page_start = (selected_ / kCardsPerPage) * kCardsPerPage;
    const int page_end = std::min(page_start + kCardsPerPage, count);

    char time_label[8] = {};
    formatCivilTime(context_->clock().localTime(), time_label, sizeof(time_label));

    const theme::Palette palette = theme::paletteFor(context_->settings().theme());
    UiRenderer renderer(context_->display(), palette);
    renderer.beginFrame();
    renderer.clearAppCanvas();
    drawAppHeader(renderer.surface(), palette, assets::kLogoHeader, "LUMA", time_label,
                  context_->network().state(), context_->network().signalStrength(),
                  context_->battery().current());
    for (int i = page_start; i < page_end; ++i) {
        const AppDescriptor* descriptor = launchableAt(i);
        if (descriptor == nullptr) {
            continue;
        }
        const int slot = i - page_start;
        drawAppCard(renderer.surface(), palette, slot % kCardColumns, slot / kCardColumns,
                    descriptor->name, descriptor->instance->accent(), i == selected_);
    }
    renderer.endFrame();
}

}  // namespace luma
