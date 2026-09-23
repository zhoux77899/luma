#pragma once

#include "luma/core/app.h"

namespace luma {

class AppManager;
struct AppDescriptor;

class LauncherApp : public App {
public:
    explicit LauncherApp(AppManager& manager);

    const char* id() const override;
    const char* name() const override;

    void onEnter(AppContext& context) override;
    void update(const InputFrame& input) override;
    void draw() override;

private:
    int launchableCount() const;
    const AppDescriptor* launchableAt(int index) const;
    void moveSelection(InputAction action, int count);

    AppManager& manager_;
    AppContext* context_ = nullptr;
    int selected_ = 0;
};

}  // namespace luma
