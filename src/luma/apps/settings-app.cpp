#include "luma/apps/settings-app.h"

#include "luma/core/app-context.h"
#include "luma/core/display.h"
#include "luma/core/settings.h"
#include "luma/ui/app-chrome.h"
#include "luma/ui/components.h"
#include "luma/ui/font.h"
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
constexpr int kPaneGap = 4;
constexpr int kOuterPad = 3;
constexpr int kInnerCardHeight = 14;
constexpr int kInnerCardGap = 2;
constexpr int kRowBoxHeight = 14;
constexpr int kBarHeight = 4;
constexpr int kBarCardHeight = 22;
constexpr int kCardPad = 3;

const char* kCategoryLabels[kCategoryCount] = {"Display", "Sound", "Network", "Power", "System"};

int centeredTextY(int box_y, int box_h) { return box_y + (box_h - font::kGlyphHeight) / 2; }

void drawDetailCard(DisplaySurface& display, const theme::Palette& palette, Rect bounds,
                    bool selected) {
    display.fillRoundRect(bounds, layout::kCardRadius, palette.canvas);
    if (selected) {
        display.drawRoundRect(bounds, layout::kCardRadius, palette.accent);
    }
}

void drawBarRow(DisplaySurface& display, const theme::Palette& palette, Rect bounds,
                const char* label, uint8_t percent, bool selected) {
    drawDetailCard(display, palette, bounds, selected);

    const int text_y = bounds.y + kCardPad;
    display.drawText({bounds.x + 4, text_y},
                     {selected ? palette.primary_text : palette.secondary_text, 1},
                     label != nullptr ? label : "");

    char value[8] = {};
    std::snprintf(value, sizeof(value), "%u%%", static_cast<unsigned>(percent));
    const int value_w = font::textWidth(value, 1);
    const int value_x = bounds.x + bounds.w - 4 - value_w;
    display.drawText({value_x, text_y}, {palette.primary_text, 1}, value);

    const int bar_x = bounds.x + 4;
    const int bar_w = bounds.w - 8;
    const int bar_y = text_y + font::kGlyphHeight + 2;
    if (bar_w > 0) {
        drawProgressBar(display, palette, {bar_x, bar_y, bar_w, kBarHeight}, percent);
    }
}

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

bool SettingsApp::isVolume() const { return category_ == kSound && detail_ == 0; }

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
    if (isVolume()) {
        const int next = static_cast<int>(settings.volume()) + delta * 10;
        if (next < 0) {
            settings.setVolume(0);
        } else if (next > 100) {
            settings.setVolume(100);
        } else {
            settings.setVolume(static_cast<uint8_t>(next));
        }
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
        if (isTheme()) {
            changeSelected(1);
        }
    }
}

void SettingsApp::detailLabelValue(int index, const char*& label, const char*& value,
                                  char* brightness, char* volume, const char* theme_label) const {
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
        label = "Volume";
        value = volume;
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
    const int outer_y = layout::kContentBoth.y + 2;
    const int outer_h = layout::kContentBoth.h - 4;
    const Rect outer{left_x, outer_y, kCategoryPaneWidth, outer_h};
    renderer.surface().fillRoundRect(outer, layout::kCardRadius, palette.card);

    const int inner_x = left_x + kOuterPad;
    const int inner_w = kCategoryPaneWidth - 2 * kOuterPad;
    const int inner_y0 = outer_y + kOuterPad;
    for (int i = 0; i < kCategoryCount; ++i) {
        const Rect card{inner_x, inner_y0 + i * (kInnerCardHeight + kInnerCardGap), inner_w,
                        kInnerCardHeight};
        const bool selected = i == category_;
        const bool focused = pane_ == Pane::Category;
        if (selected && focused) {
            renderer.surface().fillRoundRect(card, layout::kCardRadius, palette.accent);
        } else {
            renderer.surface().fillRoundRect(card, layout::kCardRadius, palette.canvas);
            if (selected) {
                renderer.surface().drawRoundRect(card, layout::kCardRadius, palette.accent);
            }
        }
        const Color text = selected && focused
                               ? palette.canvas
                               : (selected ? palette.primary_text : palette.secondary_text);
        renderer.surface().drawText({card.x + 4, centeredTextY(card.y, card.h)}, {text, 1},
                                    kCategoryLabels[i]);
    }

    const int right_x = left_x + kCategoryPaneWidth + kPaneGap;
    const int right_w = layout::kWidth - layout::kChromeInset - right_x;
    const Rect right_outer{right_x, outer_y, right_w, outer_h};
    renderer.surface().fillRoundRect(right_outer, layout::kCardRadius, palette.card);

    const int detail_x = right_x + kOuterPad;
    const int detail_w = right_w - 2 * kOuterPad;
    int row_y = outer_y + kOuterPad;

    char brightness[8] = {};
    std::snprintf(brightness, sizeof(brightness), "%u%%",
                  static_cast<unsigned>(settings.brightness()));
    char volume[8] = {};
    std::snprintf(volume, sizeof(volume), "%u%%", static_cast<unsigned>(settings.volume()));
    const char* theme_label = settings.theme() == 0 ? "Dark" : "Light";
    const int count = detailCount();
    for (int i = 0; i < count; ++i) {
        const char* label = "";
        const char* value = "";
        detailLabelValue(i, label, value, brightness, volume, theme_label);
        const bool bar = (category_ == kDisplay && i == 0) || category_ == kSound;
        const int row_h = bar ? kBarCardHeight : kRowBoxHeight;
        const Rect row{detail_x, row_y, detail_w, row_h};
        const bool selected = pane_ == Pane::Detail && i == detail_;
        if (bar) {
            const uint8_t percent = category_ == kSound ? settings.volume() : settings.brightness();
            drawBarRow(renderer.surface(), palette, row, label, percent, selected);
        } else {
            drawDetailCard(renderer.surface(), palette, row, selected);
            renderer.surface().drawText({row.x + 4, centeredTextY(row.y, row.h)},
                                        {selected ? palette.primary_text : palette.secondary_text, 1},
                                        label);
            if (value != nullptr && value[0] != '\0') {
                const int value_x = row.x + row.w - 4 - font::textWidth(value, 1);
                renderer.surface().drawText({value_x, centeredTextY(row.y, row.h)},
                                            {palette.primary_text, 1}, value);
            }
        }
        row_y += row_h + kInnerCardGap;
    }

    const KeyHint hints[] = {{"Ent", "ok"}, {"Esc", "back"}};
    drawStandardFooter(renderer, hints, 2);
    renderer.endFrame();
}

}  // namespace luma
