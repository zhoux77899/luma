#pragma once

#include "luma/core/app.h"
#include "luma/core/input-frame.h"

#include <cstddef>

namespace luma {

class AppContext;
class Diagnostics;

struct AppDescriptor {
    App* instance;
    const char* id;
    const char* name;
    char shortcut;
};

class AppManager {
public:
    static constexpr size_t kMaxApps = 8;
    static constexpr const char* kLauncherId = "launcher";

    AppManager(AppContext& context, Diagnostics& diagnostics);

    bool registerApp(const AppDescriptor& descriptor);
    bool enter(const char* id);
    void dispatch(const InputFrame& input);
    void drawIfNeeded();

    App* current() const;
    const char* currentId() const;
    size_t appCount() const;
    const AppDescriptor& appAt(size_t index) const;

private:
    App* findById(const char* id) const;
    App* findByShortcut(char shortcut) const;
    bool currentIsLauncher() const;

    AppContext& context_;
    Diagnostics& diagnostics_;
    AppDescriptor apps_[kMaxApps] = {};
    size_t app_count_ = 0;
    App* current_ = nullptr;
    bool needs_draw_ = false;
};

}  // namespace luma
