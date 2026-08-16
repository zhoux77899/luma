#pragma once

#include "luma/core/app.h"

namespace luma {

class AppManager;

class LauncherApp : public App {
public:
    explicit LauncherApp(AppManager& manager);

    const char* id() const override;
    const char* name() const override;

    void onEnter(AppContext& context) override;
    void update(const InputFrame& input) override;
    void draw() override;

private:
    static constexpr int kMaxLaunchable = 8;

    int launchableCount() const;
    void collectLaunchable(const char** ids, const char** names) const;
    void moveSelection(InputAction action, int count);

    AppManager& manager_;
    AppContext* context_ = nullptr;
    int selected_ = 0;
};

}  // namespace luma
