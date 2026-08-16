#include "luma/apps/settings-app.h"

#include "luma/core/app-context.h"
#include "luma/core/display.h"
#include "luma/core/settings.h"
#include "luma/ui/app-chrome.h"
#include "luma/ui/components.h"
#include "luma/ui/layout.h"
#include "luma/ui/renderer.h"
#include "luma/ui/theme.h"

#include <cstdint>
#include <cstdio>

namespace luma {
namespace {

enum Category : int {
    kDisplay = 0,
    kSound = 1,
    kNetwork = 2,
    kPower = 3,
    kSystem = 4,
    kCategoryCount = 5
};

constexpr int kCategoryPaneWidth = 88;
constexpr int kPaneSplitWidth = 1;
constexpr int kRowHeight = 16;
constexpr int kRowBoxHeight = 14;
constexpr int kRowStartPad = 4;

const char* kCategoryLabels[kCategoryCount] = {"Display", "Sound", "Network", "Power", "System"};

}  // namespace

const char* SettingsApp::id() const { return "settings"; }
const char* SettingsApp::name() const { return "SETTINGS"; }

void SettingsApp::onEnter(AppContext& context) {
    context_ = &context;
    pane_ = Pane::Category;
    category_ = kDisplay;
    detail_ = 0;
}

void SettingsApp::onExit() {
    if (context_ != nullptr) {
        context_->settings().flushNow();
    }
}

void SettingsApp::applyImmediate() {
    if (context_ == nullptr) {
        return;
    }
    context_->display().setBrightness(context_->settings().brightness());
    context_->requestUiSound();
    context_->requestRedraw();
}

int SettingsApp::detailCount() const {
    if (category_ == kDisplay || category_ == kNetwork) {
        return 2;
    }
    return 1;
}

bool SettingsApp::isBrightness() const { return category_ == kDisplay && detail_ == 0; }

bool SettingsApp::isSoundItem() const { return category_ == kSound && detail_ == 0; }

bool SettingsApp::isTheme() const { return category_ == kDisplay && detail_ == 1; }

bool SettingsApp::isAbout() const { return category_ == kSystem && detail_ == 0; }

void SettingsApp::changeSelected(int delta) {
    if (context_ == nullptr || pane_ != Pane::Detail) {
        return;
    }
    Settings& settings = context_->settings();
    if (isBrightness()) {
        const int next = static_cast<int>(settings.brightness()) + delta * 10;
        if (next < 0) {
            settings.setBrightness(0);
        } else if (next > 100) {
            settings.setBrightness(100);
        } else {
            settings.setBrightness(static_cast<uint8_t>(next));
        }
        applyImmediate();
        return;
    }
    if (isSoundItem()) {
        settings.setSound(!settings.sound());
        applyImmediate();
        return;
    }
    if (isTheme()) {
        settings.setTheme(settings.theme() == 0 ? 1 : 0);
        applyImmediate();
    }
}

bool SettingsApp::handleValueKey(const InputFrame& input) {
    if (pane_ != Pane::Detail) {
        return false;
    }
    for (uint8_t i = 0; i < input.textLength; ++i) {
        if (input.text[i] == '-' || input.text[i] == '_') {
            changeSelected(-1);
            return true;
        }
        if (input.text[i] == '=' || input.text[i] == '+') {
            changeSelected(1);
            return true;
        }
    }
    return false;
}

void SettingsApp::update(const InputFrame& input) {
    if (context_ == nullptr) {
        return;
    }

    if (input.action == InputAction::Back) {
        if (pane_ == Pane::Detail) {
            pane_ = Pane::Category;
            context_->consumeBack();
            context_->requestRedraw();
        }
        return;
    }

    if (pane_ == Pane::Category) {
        if (input.action == InputAction::Up && category_ > 0) {
            --category_;
            detail_ = 0;
            context_->requestRedraw();
            return;
        }
        if (input.action == InputAction::Down && category_ + 1 < kCategoryCount) {
            ++category_;
            detail_ = 0;
            context_->requestRedraw();
            return;
        }
        if (input.action == InputAction::Confirm) {
            pane_ = Pane::Detail;
            context_->requestRedraw();
        }
        return;
    }

    if (input.action == InputAction::Up && detail_ > 0) {
        --detail_;
        context_->requestRedraw();
        return;
    }
    if (input.action == InputAction::Down && detail_ + 1 < detailCount()) {
        ++detail_;
        context_->requestRedraw();
        return;
    }
    if (input.action == InputAction::Left) {
        changeSelected(-1);
        return;
    }
    if (input.action == InputAction::Right) {
        changeSelected(1);
        return;
    }
    if (handleValueKey(input)) {
        return;
    }
    if (input.action == InputAction::Confirm) {
        if (isAbout()) {
            context_->requestEnter("about");
            return;
        }
        if (isSoundItem() || isTheme()) {
            changeSelected(1);
        }
    }
}

void SettingsApp::detailLabelValue(int index, const char*& label, const char*& value,
                                  char* brightness, const char* sound,
                                  const char* theme_label) const {
    label = "";
    value = "";
    if (category_ == kDisplay) {
        if (index == 0) {
            label = "Brightness";
            value = brightness;
        } else {
            label = "Theme";
            value = theme_label;
        }
        return;
    }
    if (category_ == kSound) {
        label = "Sound";
        value = sound;
        return;
    }
    if (category_ == kNetwork) {
        if (index == 0) {
            label = "Wi-Fi";
        } else {
            label = "Time zone";
        }
        value = "--";
        return;
    }
    if (category_ == kPower) {
        label = "Battery";
        return;
    }
    label = "About";
}

void SettingsApp::draw() {
    if (context_ == nullptr) {
        return;
    }

    Settings& settings = context_->settings();
    const theme::Palette palette = theme::paletteFor(settings.theme());
    UiRenderer renderer(context_->display(), palette);
    renderer.beginFrame();
    renderer.clearAppCanvas();
    drawStandardHeader(*context_, renderer, name());

    const int left_x = layout::kChromeInset;
    const int split_x = left_x + kCategoryPaneWidth;
    const int right_x = split_x + kPaneSplitWidth + 1;
    const int right_w = layout::kWidth - layout::kChromeInset - right_x;
    const int row_y0 = layout::kContentBoth.y + kRowStartPad;

    renderer.surface().fillRect({split_x, row_y0, kPaneSplitWidth, layout::kContentBoth.h - 8},
                                palette.secondary_text);

    for (int i = 0; i < kCategoryCount; ++i) {
        const Rect row{left_x, row_y0 + i * kRowHeight, kCategoryPaneWidth, kRowBoxHeight};
        drawMenuItem(renderer.surface(), palette, row, kCategoryLabels[i], i == category_, nullptr,
                     pane_ == Pane::Category);
    }

    char brightness[8] = {};
    std::snprintf(brightness, sizeof(brightness), "%u%%",
                  static_cast<unsigned>(settings.brightness()));
    const char* sound = settings.sound() ? "On" : "Off";
    const char* theme_label = settings.theme() == 0 ? "Dark" : "Light";
    const int count = detailCount();
    for (int i = 0; i < count; ++i) {
        const char* label = "";
        const char* value = "";
        detailLabelValue(i, label, value, brightness, sound, theme_label);
        const Rect row{right_x, row_y0 + i * kRowHeight, right_w, kRowBoxHeight};
        const bool selected = pane_ == Pane::Detail && i == detail_;
        drawMenuItem(renderer.surface(), palette, row, label, selected, value, true);
    }

    const KeyHint hints[] = {{"Ent", "ok"}, {"Esc", "back"}};
    drawStandardFooter(renderer, hints, 2);
    renderer.endFrame();
}

}  // namespace luma
