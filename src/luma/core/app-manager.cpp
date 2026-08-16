#include "luma/core/app-manager.h"

#include "luma/core/app-context.h"
#include "luma/core/diagnostics.h"

#include <cstdio>
#include <cstring>

namespace luma {
namespace {

void emitApp(Diagnostics& diagnostics, const char* verb, const char* id) {
    char message[48];
    std::snprintf(message, sizeof(message), "%s %s", verb, id);
    diagnostics.emit("APP", message);
}

}  // namespace

AppManager::AppManager(AppContext& context, Diagnostics& diagnostics)
    : context_(context), diagnostics_(diagnostics) {}

bool AppManager::registerApp(const AppDescriptor& descriptor) {
    if (descriptor.instance == nullptr || descriptor.id == nullptr || app_count_ >= kMaxApps) {
        return false;
    }
    if (findById(descriptor.id) != nullptr) {
        return false;
    }
    apps_[app_count_++] = descriptor;
    return true;
}

bool AppManager::enter(const char* id) {
    App* next = findById(id);
    if (next == nullptr || next == current_) {
        return next == current_ && next != nullptr;
    }

    if (current_ != nullptr) {
        emitApp(diagnostics_, "exit", current_->id());
        current_->onExit();
    }

    current_ = next;
    emitApp(diagnostics_, "enter", current_->id());
    current_->onEnter(context_);
    needs_draw_ = true;
    return true;
}

void AppManager::dispatch(const InputFrame& input) {
    if (current_ != nullptr && currentIsLauncher() && input.textLength == 1) {
        App* target = findByShortcut(input.text[0]);
        if (target != nullptr) {
            enter(target->id());
            return;
        }
    }

    if (input.action == InputAction::Back) {
        if (current_ != nullptr && !currentIsLauncher()) {
            enter(kLauncherId);
        }
        return;
    }

    if (current_ != nullptr) {
        current_->update(input);
    }
}

void AppManager::drawIfNeeded() {
    if (current_ == nullptr) {
        return;
    }
    if (needs_draw_ || context_.takeRedrawRequest()) {
        current_->draw();
        needs_draw_ = false;
    }
}

App* AppManager::current() const { return current_; }

const char* AppManager::currentId() const {
    return current_ != nullptr ? current_->id() : "";
}

App* AppManager::findById(const char* id) const {
    if (id == nullptr) {
        return nullptr;
    }
    for (size_t i = 0; i < app_count_; ++i) {
        if (std::strcmp(apps_[i].id, id) == 0) {
            return apps_[i].instance;
        }
    }
    return nullptr;
}

App* AppManager::findByShortcut(char shortcut) const {
    if (shortcut == '\0') {
        return nullptr;
    }
    for (size_t i = 0; i < app_count_; ++i) {
        if (apps_[i].shortcut == shortcut) {
            return apps_[i].instance;
        }
    }
    return nullptr;
}

bool AppManager::currentIsLauncher() const {
    return current_ != nullptr && std::strcmp(current_->id(), kLauncherId) == 0;
}

}  // namespace luma
