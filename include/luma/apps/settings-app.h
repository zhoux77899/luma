#pragma once

#include "luma/core/app.h"

namespace luma {

class SettingsApp : public App {
public:
    const char* id() const override;
    const char* name() const override;

    void onEnter(AppContext& context) override;
    void onExit() override;
    void update(const InputFrame& input) override;
    void draw() override;

private:
    void applyImmediate();
    void changeSelected(int delta);
    bool handleValueKey(const InputFrame& input);

    AppContext* context_ = nullptr;
    int selected_ = 0;
};

}  // namespace luma
