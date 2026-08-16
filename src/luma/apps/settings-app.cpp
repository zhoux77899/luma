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

enum Row : int { kBrightness = 0, kSound = 1, kTheme = 2, kAbout = 3, kRowCount = 4 };

}  // namespace

const char* SettingsApp::id() const { return "settings"; }
const char* SettingsApp::name() const { return "SETTINGS"; }

void SettingsApp::onEnter(AppContext& context) {
    context_ = &context;
    selected_ = 0;
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

void SettingsApp::changeSelected(int delta) {
    if (context_ == nullptr) {
        return;
    }
    Settings& settings = context_->settings();
    if (selected_ == kBrightness) {
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
    if (selected_ == kSound) {
        settings.setSound(!settings.sound());
        applyImmediate();
        return;
    }
    if (selected_ == kTheme) {
        settings.setTheme(settings.theme() == 0 ? 1 : 0);
        applyImmediate();
    }
}

bool SettingsApp::handleValueKey(const InputFrame& input) {
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

    if (input.action == InputAction::Up && selected_ > 0) {
        --selected_;
        context_->requestRedraw();
        return;
    }
    if (input.action == InputAction::Down && selected_ + 1 < kRowCount) {
        ++selected_;
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
        if (selected_ == kAbout) {
            context_->requestEnter("about");
            return;
        }
        if (selected_ == kSound || selected_ == kTheme) {
            changeSelected(1);
        }
    }
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

    char brightness[8] = {};
    std::snprintf(brightness, sizeof(brightness), "%u%%",
                  static_cast<unsigned>(settings.brightness()));
    const char* sound = settings.sound() ? "On" : "Off";
    const char* theme_label = settings.theme() == 0 ? "Dark" : "Light";
    const char* labels[kRowCount] = {"Brightness", "Sound", "Theme", "About"};
    const char* values[kRowCount] = {brightness, sound, theme_label, ""};

    for (int i = 0; i < kRowCount; ++i) {
        const Rect row{layout::kChromeInset, layout::kContentBoth.y + 4 + i * 16,
                       layout::kWidth - 2 * layout::kChromeInset, 14};
        drawMenuItem(renderer.surface(), palette, row, labels[i], i == selected_, values[i]);
    }

    const KeyHint hints[] = {{"Ent", "ok"}, {"Esc", "back"}};
    drawStandardFooter(renderer, hints, 2);
    renderer.endFrame();
}

}  // namespace luma
