#include "luma/apps/settings-app.h"

#include "luma/core/app-context.h"
#include "luma/core/clock.h"
#include "luma/core/display.h"
#include "luma/core/network.h"
#include "luma/core/settings.h"
#include "luma/core/time-zone.h"
#include "luma/ui/app-chrome.h"
#include "luma/ui/components.h"
#include "luma/ui/font.h"
#include "luma/ui/layout.h"
#include "luma/ui/renderer.h"
#include "luma/ui/theme.h"

#include <cstdint>
#include <cstdio>
#include <cstring>

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

void drawEditorRow(DisplaySurface& display, const theme::Palette& palette, Rect bounds,
                   const char* label, const char* value, bool selected) {
    drawDetailCard(display, palette, bounds, selected);
    display.drawText({bounds.x + 4, centeredTextY(bounds.y, bounds.h)},
                     {selected ? palette.primary_text : palette.secondary_text, 1},
                     label != nullptr ? label : "");
    if (value != nullptr && value[0] != '\0') {
        const int value_x = bounds.x + bounds.w - 4 - font::textWidth(value, 1);
        display.drawText({value_x, centeredTextY(bounds.y, bounds.h)}, {palette.primary_text, 1},
                         value);
    }
}

}  // namespace

const char* SettingsApp::id() const { return "settings"; }
const char* SettingsApp::name() const { return "SETTINGS"; }
Color SettingsApp::accent() const { return theme::kTsuyukusa; }

void SettingsApp::onEnter(AppContext& context) {
    context_ = &context;
    pane_ = Pane::Category;
    editor_ = Editor::None;
    category_ = kDisplay;
    detail_ = 0;
    editor_index_ = 0;
    password_len_ = 0;
    password_[0] = '\0';
    pending_ssid_[0] = '\0';
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

bool SettingsApp::isWifi() const { return category_ == kNetwork && detail_ == 0; }

bool SettingsApp::isTimeZone() const { return category_ == kNetwork && detail_ == 1; }

const char* SettingsApp::networkStateLabel() const {
    if (context_ == nullptr) {
        return "Unknown";
    }
    switch (context_->network().state()) {
        case NetworkState::Connecting:
            return "Connecting";
        case NetworkState::Connected:
            return context_->network().connectedSsid();
        case NetworkState::Failed:
            return "Failed";
        case NetworkState::Unknown:
            return "Unknown";
        case NetworkState::Disconnected:
        default:
            return "Disconnected";
    }
}

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

int SettingsApp::wifiRowCount() const {
    if (context_ == nullptr) {
        return 2;
    }
    return 2 + context_->network().profileCount() + context_->network().publicScanCount();
}

SettingsApp::WifiRowKind SettingsApp::wifiRowKind(int index, int& payload) const {
    payload = 0;
    if (index <= 0) {
        return WifiRowKind::Status;
    }
    const int profiles = context_ != nullptr ? context_->network().profileCount() : 0;
    if (index <= profiles) {
        payload = index - 1;
        return WifiRowKind::Profile;
    }
    if (index == profiles + 1) {
        return WifiRowKind::ScanAction;
    }
    payload = index - profiles - 2;
    return WifiRowKind::ScanHit;
}

void SettingsApp::updateWifiEditor(const InputFrame& input) {
    if (input.action == InputAction::Back) {
        editor_ = Editor::None;
        context_->consumeBack();
        context_->requestRedraw();
        return;
    }

    Network& network = context_->network();
    const int count = wifiRowCount();
    if (input.action == InputAction::Up && editor_index_ > 0) {
        --editor_index_;
        context_->requestRedraw();
        return;
    }
    if (input.action == InputAction::Down && editor_index_ + 1 < count) {
        ++editor_index_;
        context_->requestRedraw();
        return;
    }

    int payload = 0;
    const WifiRowKind kind = wifiRowKind(editor_index_, payload);
    if (input.action == InputAction::Delete && kind == WifiRowKind::Profile) {
        network.deleteProfile(payload);
        if (editor_index_ >= wifiRowCount()) {
            editor_index_ = wifiRowCount() - 1;
        }
        context_->requestRedraw();
        return;
    }
    if (input.action != InputAction::Confirm) {
        return;
    }
    if (kind == WifiRowKind::ScanAction) {
        network.startScan();
        context_->requestRedraw();
        return;
    }
    if (kind == WifiRowKind::Profile) {
        network.connectProfile(payload);
        context_->requestRedraw();
        return;
    }
    if (kind == WifiRowKind::ScanHit) {
        WifiScanHit hit;
        if (!network.publicScanAt(payload, hit)) {
            return;
        }
        std::snprintf(pending_ssid_, sizeof(pending_ssid_), "%s", hit.ssid);
        if (!hit.encrypted) {
            network.connect(hit.ssid, "");
            context_->requestRedraw();
            return;
        }
        password_len_ = 0;
        password_[0] = '\0';
        editor_ = Editor::Password;
        context_->requestRedraw();
    }
}

void SettingsApp::updatePasswordEditor(const InputFrame& input) {
    if (input.action == InputAction::Back) {
        editor_ = Editor::Wifi;
        context_->consumeBack();
        context_->requestRedraw();
        return;
    }
    if (input.action == InputAction::Delete && password_len_ > 0) {
        --password_len_;
        password_[password_len_] = '\0';
        context_->requestRedraw();
        return;
    }
    if (input.action == InputAction::Confirm) {
        context_->network().connect(pending_ssid_, password_);
        editor_ = Editor::Wifi;
        context_->requestRedraw();
        return;
    }
    for (uint8_t i = 0; i < input.textLength && password_len_ + 1 < sizeof(password_); ++i) {
        password_[password_len_++] = input.text[i];
        password_[password_len_] = '\0';
        context_->requestRedraw();
    }
}

void SettingsApp::updateTimeZoneEditor(const InputFrame& input) {
    if (input.action == InputAction::Back) {
        editor_ = Editor::None;
        context_->consumeBack();
        context_->requestRedraw();
        return;
    }
    if (input.action == InputAction::Up && tz_index_ > 0) {
        --tz_index_;
        context_->requestRedraw();
        return;
    }
    if (input.action == InputAction::Down && tz_index_ + 1 < timeZoneCount()) {
        ++tz_index_;
        context_->requestRedraw();
        return;
    }
    if (input.action == InputAction::Confirm) {
        context_->clock().setTimeZone(timeZoneIdAt(tz_index_));
        editor_ = Editor::None;
        context_->requestRedraw();
    }
}

void SettingsApp::updateSplitPane(const InputFrame& input) {
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
            return;
        }
        if (isWifi()) {
            editor_ = Editor::Wifi;
            editor_index_ = 0;
            context_->requestRedraw();
            return;
        }
        if (isTimeZone()) {
            editor_ = Editor::TimeZone;
            tz_index_ = 0;
            const char* current = context_->clock().timeZoneId();
            for (int i = 0; i < timeZoneCount(); ++i) {
                if (std::strcmp(timeZoneIdAt(i), current) == 0) {
                    tz_index_ = i;
                    break;
                }
            }
            context_->requestRedraw();
        }
    }
}

void SettingsApp::update(const InputFrame& input) {
    if (context_ == nullptr) {
        return;
    }
    if (editor_ == Editor::Wifi) {
        updateWifiEditor(input);
        return;
    }
    if (editor_ == Editor::Password) {
        updatePasswordEditor(input);
        return;
    }
    if (editor_ == Editor::TimeZone) {
        updateTimeZoneEditor(input);
        return;
    }
    updateSplitPane(input);
}

void SettingsApp::detailLabelValue(int index, const char*& label, const char*& value,
                                  char* brightness, char* volume, const char* theme_label,
                                  char* wifi, char* zone) const {
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
            value = wifi;
        } else {
            label = "Time zone";
            value = zone;
        }
        return;
    }
    if (category_ == kPower) {
        label = "Battery";
        return;
    }
    label = "About";
}

void SettingsApp::drawSplitPane() {
    Settings& settings = context_->settings();
    const theme::Palette palette = theme::paletteFor(settings.theme(), accent());
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
    char wifi[24] = {};
    std::snprintf(wifi, sizeof(wifi), "%s", networkStateLabel());
    char zone[24] = {};
    std::snprintf(zone, sizeof(zone), "%s", context_->clock().timeZoneId());
    const int count = detailCount();
    for (int i = 0; i < count; ++i) {
        const char* label = "";
        const char* value = "";
        detailLabelValue(i, label, value, brightness, volume, theme_label, wifi, zone);
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

void SettingsApp::drawWifiEditor() {
    const theme::Palette palette = theme::paletteFor(context_->settings().theme(), accent());
    UiRenderer renderer(context_->display(), palette);
    renderer.beginFrame();
    renderer.clearAppCanvas();
    drawStandardHeader(*context_, renderer, name());

    Network& network = context_->network();
    const int count = wifiRowCount();
    int row_y = layout::kContentBoth.y + 2;
    const int row_w = layout::kWidth - 2 * layout::kChromeInset;
    for (int i = 0; i < count; ++i) {
        int payload = 0;
        const WifiRowKind kind = wifiRowKind(i, payload);
        const char* label = "";
        const char* value = "";
        char scan_label[36] = {};
        if (kind == WifiRowKind::Status) {
            label = "Status";
            value = networkStateLabel();
        } else if (kind == WifiRowKind::Profile) {
            label = network.profileSsid(payload);
            value = "Saved";
        } else if (kind == WifiRowKind::ScanAction) {
            label = network.scanInProgress() ? "Scanning" : "Scan";
        } else {
            WifiScanHit hit;
            if (network.publicScanAt(payload, hit)) {
                std::snprintf(scan_label, sizeof(scan_label), "%s", hit.ssid);
                label = scan_label;
                value = hit.encrypted ? "Key" : "Open";
            }
        }
        const Rect row{layout::kChromeInset, row_y, row_w, kRowBoxHeight};
        drawEditorRow(renderer.surface(), palette, row, label, value, i == editor_index_);
        row_y += kRowBoxHeight + kInnerCardGap;
        if (row_y > layout::kFooter.y - kRowBoxHeight) {
            break;
        }
    }

    const KeyHint hints[] = {{"Ent", "ok"}, {"Del", "forget"}, {"Esc", "back"}};
    drawStandardFooter(renderer, hints, 3);
    renderer.endFrame();
}

void SettingsApp::drawPasswordEditor() {
    const theme::Palette palette = theme::paletteFor(context_->settings().theme(), accent());
    UiRenderer renderer(context_->display(), palette);
    renderer.beginFrame();
    renderer.clearAppCanvas();
    drawStandardHeader(*context_, renderer, name());

    renderer.surface().drawText({layout::kChromeInset, layout::kContentBoth.y + 8},
                                {palette.secondary_text, 1}, pending_ssid_);
    char masked[64] = {};
    for (uint8_t i = 0; i < password_len_ && i + 1 < sizeof(masked); ++i) {
        masked[i] = '*';
    }
    renderer.surface().drawText({layout::kChromeInset, layout::kContentBoth.y + 24},
                                {palette.primary_text, 1}, masked);
    const KeyHint hints[] = {{"Ent", "join"}, {"Esc", "back"}};
    drawStandardFooter(renderer, hints, 2);
    renderer.endFrame();
}

void SettingsApp::drawTimeZoneEditor() {
    const theme::Palette palette = theme::paletteFor(context_->settings().theme(), accent());
    UiRenderer renderer(context_->display(), palette);
    renderer.beginFrame();
    renderer.clearAppCanvas();
    drawStandardHeader(*context_, renderer, name());

    const int visible = 5;
    int start = tz_index_ - visible / 2;
    if (start < 0) {
        start = 0;
    }
    if (start + visible > timeZoneCount()) {
        start = timeZoneCount() - visible;
    }
    if (start < 0) {
        start = 0;
    }
    int row_y = layout::kContentBoth.y + 2;
    const int row_w = layout::kWidth - 2 * layout::kChromeInset;
    for (int i = 0; i < visible && start + i < timeZoneCount(); ++i) {
        const int index = start + i;
        const Rect row{layout::kChromeInset, row_y, row_w, kRowBoxHeight};
        drawEditorRow(renderer.surface(), palette, row, timeZoneIdAt(index), nullptr,
                      index == tz_index_);
        row_y += kRowBoxHeight + kInnerCardGap;
    }

    const KeyHint hints[] = {{"Ent", "set"}, {"Esc", "back"}};
    drawStandardFooter(renderer, hints, 2);
    renderer.endFrame();
}

void SettingsApp::draw() {
    if (context_ == nullptr) {
        return;
    }
    if (editor_ == Editor::Wifi) {
        drawWifiEditor();
        return;
    }
    if (editor_ == Editor::Password) {
        drawPasswordEditor();
        return;
    }
    if (editor_ == Editor::TimeZone) {
        drawTimeZoneEditor();
        return;
    }
    drawSplitPane();
}

}  // namespace luma
