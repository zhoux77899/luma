#pragma once

#include "luma/core/app.h"

namespace luma {

class SettingsApp : public App {
public:
    const char* id() const override;
    const char* name() const override;
    Color accent() const override;

    void onEnter(AppContext& context) override;
    void onExit() override;
    void update(const InputFrame& input) override;
    void draw() override;

private:
    enum class Pane : int { Category, Detail };

    void applyImmediate();
    void changeSelected(int delta);
    bool handleValueKey(const InputFrame& input);
    int detailCount() const;
    bool isBrightness() const;
    bool isVolume() const;
    bool isTheme() const;
    bool isAbout() const;
    void detailLabelValue(int index, const char*& label, const char*& value, char* brightness,
                          char* volume, const char* theme_label) const;

    AppContext* context_ = nullptr;
    Pane pane_ = Pane::Category;
    int category_ = 0;
    int detail_ = 0;
};

}  // namespace luma
