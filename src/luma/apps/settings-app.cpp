#include "luma/apps/settings-app.h"

#include "luma/assets/wifi-icons.h"
#include "luma/core/app-context.h"
#include "luma/core/battery.h"
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
    kTime = 3,
    kBattery = 4,
    kSystem = 5,
    kCategoryCount = 6
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

const char* kCategoryLabels[kCategoryCount] = {"Display", "Sound", "Network", "Time", "Battery",
                                              "System"};
constexpr int kCategoryVisible = 5;

constexpr int kWifiSectionCount = 3;
const char* kWifiSectionLabels[kWifiSectionCount] = {"Status", "Saved", "Scan"};
constexpr int kListVisible = 5;
constexpr int kScrollbarWidth = 2;

int centeredTextY(int box_y, int box_h) { return box_y + (box_h - font::kGlyphHeight) / 2; }

void copyPrefixFitting(const char* src, char* dst, size_t dst_size, int max_width) {
    if (dst == nullptr || dst_size == 0) {
        return;
    }
    dst[0] = '\0';
    if (src == nullptr) {
        return;
    }
    const char* cursor = src;
    const char* keep = src;
    uint32_t code = 0;
    int width = 0;
    while (font::nextCodepoint(cursor, code)) {
        const int glyph_w = font::glyphFor(code, false).width;
        if (width + glyph_w > max_width) {
            break;
        }
        width += glyph_w;
        keep = cursor;
    }
    size_t n = static_cast<size_t>(keep - src);
    if (n + 1 > dst_size) {
        n = dst_size - 1;
    }
    std::memcpy(dst, src, n);
    dst[n] = '\0';
}

int visibleMaskCount(int total, int max_width) {
    if (total <= 0 || max_width <= 0) {
        return 0;
    }
    const int star_w = font::glyphWidth(false);
    int visible = star_w > 0 ? max_width / star_w : 0;
    if (visible > total) {
        visible = total;
    }
    return visible;
}

int listWindowStart(int selected, int count, int visible) {
    int start = selected - visible / 2;
    if (start < 0) {
        start = 0;
    }
    if (start + visible > count) {
        start = count - visible;
    }
    if (start < 0) {
        start = 0;
    }
    return start;
}

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

constexpr int kRowTextPad = 4;
constexpr int kListIconGap = 2;

bool savedSignalStrength(const Network& network, int index, SignalStrength& out) {
    const char* ssid = network.profileSsid(index);
    if (ssid == nullptr || ssid[0] == '\0') {
        return false;
    }
    if (network.state() == NetworkState::Connected &&
        std::strcmp(ssid, network.connectedSsid()) == 0) {
        out = network.signalStrength();
        return out != SignalStrength::None;
    }
    const int hits = network.publicScanCount();
    for (int i = 0; i < hits; ++i) {
        WifiScanHit hit;
        if (network.publicScanAt(i, hit) && std::strcmp(hit.ssid, ssid) == 0) {
            out = signalStrengthFromRssi(hit.rssi);
            return true;
        }
    }
    return false;
}

void drawSsidIconRow(DisplaySurface& display, const theme::Palette& palette, Rect bounds,
                     const char* ssid, bool locked, bool show_wifi, SignalStrength strength,
                     bool selected) {
    drawDetailCard(display, palette, bounds, selected);
    const int icon_y = bounds.y + (bounds.h - assets::kWifiListIconSize) / 2;
    const int wifi_x = bounds.x + bounds.w - kRowTextPad - assets::kWifiListIconSize;
    const int lock_x =
        show_wifi ? wifi_x - kListIconGap - assets::kWifiListIconSize : wifi_x;
    const int ssid_x = bounds.x + kRowTextPad;
    const int ssid_max = lock_x - kListIconGap - ssid_x;
    char label[40] = {};
    ellipsizeToWidth(ssid, label, sizeof(label), ssid_max);
    display.drawText({ssid_x, centeredTextY(bounds.y, bounds.h)},
                     {selected ? palette.primary_text : palette.secondary_text, 1}, label);
    drawLockGlyph(display, palette, {lock_x, icon_y}, locked);
    if (show_wifi) {
        drawWifiListGlyph(display, palette, {wifi_x, icon_y}, strength);
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
    wifi_section_ = WifiSection::Status;
    category_ = kDisplay;
    detail_ = 0;
    editor_index_ = 0;
    tz_section_ = 0;
    tz_index_ = 0;
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
    if (category_ == kDisplay) {
        return 2;
    }
    if (category_ == kBattery) {
        return 0;
    }
    return 1;
}

bool SettingsApp::isBrightness() const { return category_ == kDisplay && detail_ == 0; }

bool SettingsApp::isVolume() const { return category_ == kSound && detail_ == 0; }

bool SettingsApp::isTheme() const { return category_ == kDisplay && detail_ == 1; }

bool SettingsApp::isAbout() const { return category_ == kSystem && detail_ == 0; }

bool SettingsApp::isWifi() const { return category_ == kNetwork && detail_ == 0; }

bool SettingsApp::isTimeZone() const { return category_ == kTime && detail_ == 0; }

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

const char* SettingsApp::networkStateName() const {
    if (context_ == nullptr) {
        return "Unknown";
    }
    switch (context_->network().state()) {
        case NetworkState::Connecting:
            return "Connecting";
        case NetworkState::Connected:
            return "Connected";
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

int SettingsApp::wifiSectionCount() const { return kWifiSectionCount; }

int SettingsApp::statusRowCount() const {
    if (context_ != nullptr && context_->network().state() == NetworkState::Connected) {
        return 5;
    }
    return 1;
}

int SettingsApp::scanRowCount() const {
    if (context_ == nullptr) {
        return 1;
    }
    if (context_->network().scanInProgress()) {
        return 1;
    }
    const int hits = context_->network().publicScanCount();
    return hits > 0 ? hits : 1;
}

int SettingsApp::wifiDetailCount() const {
    if (wifi_section_ == WifiSection::Status) {
        return statusRowCount();
    }
    if (wifi_section_ == WifiSection::Saved) {
        const int profiles = context_ != nullptr ? context_->network().profileCount() : 0;
        return profiles > 0 ? profiles : 0;
    }
    return scanRowCount();
}

void SettingsApp::enterWifiStatusDetail() {
    wifi_section_ = WifiSection::Status;
    pane_ = Pane::Detail;
    editor_ = Editor::Wifi;
    editor_index_ = 0;
}

void SettingsApp::updateWifiEditor(const InputFrame& input) {
    if (input.action == InputAction::Back) {
        if (pane_ == Pane::Detail) {
            pane_ = Pane::Category;
        } else {
            editor_ = Editor::None;
            pane_ = Pane::Detail;
        }
        context_->consumeBack();
        context_->requestRedraw();
        return;
    }

    if (pane_ == Pane::Category) {
        if (input.action == InputAction::Up && static_cast<int>(wifi_section_) > 0) {
            wifi_section_ = static_cast<WifiSection>(static_cast<int>(wifi_section_) - 1);
            editor_index_ = 0;
            context_->requestRedraw();
            return;
        }
        if (input.action == InputAction::Down &&
            static_cast<int>(wifi_section_) + 1 < wifiSectionCount()) {
            wifi_section_ = static_cast<WifiSection>(static_cast<int>(wifi_section_) + 1);
            editor_index_ = 0;
            context_->requestRedraw();
            return;
        }
        if (input.action == InputAction::Confirm) {
            pane_ = Pane::Detail;
            editor_index_ = 0;
            if (wifi_section_ == WifiSection::Scan) {
                context_->network().startScan();
            }
            context_->requestRedraw();
        }
        return;
    }

    Network& network = context_->network();
    const int count = wifiDetailCount();
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

    if (wifi_section_ == WifiSection::Saved && input.action == InputAction::Delete &&
        editor_index_ >= 0 && editor_index_ < network.profileCount()) {
        network.deleteProfile(editor_index_);
        if (editor_index_ >= wifiDetailCount() && editor_index_ > 0) {
            --editor_index_;
        }
        context_->requestRedraw();
        return;
    }
    if (input.action != InputAction::Confirm) {
        return;
    }

    if (wifi_section_ == WifiSection::Status) {
        if (network.state() == NetworkState::Connected && editor_index_ == 4) {
            network.disconnect();
            editor_index_ = 0;
            context_->requestRedraw();
        }
        return;
    }
    if (wifi_section_ == WifiSection::Saved) {
        if (editor_index_ >= 0 && editor_index_ < network.profileCount()) {
            network.connectProfile(editor_index_);
            enterWifiStatusDetail();
            context_->requestRedraw();
        }
        return;
    }
    if (network.scanInProgress() || network.publicScanCount() == 0) {
        return;
    }
    WifiScanHit hit;
    if (!network.publicScanAt(editor_index_, hit)) {
        return;
    }
    std::snprintf(pending_ssid_, sizeof(pending_ssid_), "%s", hit.ssid);
    if (!hit.encrypted) {
        network.connect(hit.ssid, "");
        enterWifiStatusDetail();
        context_->requestRedraw();
        return;
    }
    password_len_ = 0;
    password_[0] = '\0';
    editor_ = Editor::Password;
    context_->requestRedraw();
}

void SettingsApp::updatePasswordEditor(const InputFrame& input) {
    if (input.action == InputAction::Back) {
        editor_ = Editor::Wifi;
        pane_ = Pane::Detail;
        wifi_section_ = WifiSection::Scan;
        password_len_ = 0;
        password_[0] = '\0';
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
        password_len_ = 0;
        password_[0] = '\0';
        enterWifiStatusDetail();
        context_->requestRedraw();
        return;
    }
    if (input.action == InputAction::Up || input.action == InputAction::Down) {
        return;
    }
    for (uint8_t i = 0; i < input.textLength && password_len_ + 1 < sizeof(password_); ++i) {
        password_[password_len_++] = input.text[i];
        password_[password_len_] = '\0';
        context_->requestRedraw();
    }
}

void SettingsApp::enterTimeZoneEditor() {
    editor_ = Editor::TimeZone;
    pane_ = Pane::Category;
    const char* current = context_->clock().timeZoneId();
    tz_section_ = timeZoneSectionOf(current);
    tz_index_ = 0;
    const int count = timeZoneCountInSection(tz_section_);
    for (int i = 0; i < count; ++i) {
        if (std::strcmp(timeZoneIdInSection(tz_section_, i), current) == 0) {
            tz_index_ = i;
            break;
        }
    }
}

void SettingsApp::updateTimeZoneEditor(const InputFrame& input) {
    if (input.action == InputAction::Back) {
        if (pane_ == Pane::Detail) {
            pane_ = Pane::Category;
        } else {
            editor_ = Editor::None;
            pane_ = Pane::Detail;
        }
        context_->consumeBack();
        context_->requestRedraw();
        return;
    }

    if (pane_ == Pane::Category) {
        if (input.action == InputAction::Up && tz_section_ > 0) {
            --tz_section_;
            tz_index_ = 0;
            context_->requestRedraw();
            return;
        }
        if (input.action == InputAction::Down && tz_section_ + 1 < timeZoneSectionCount()) {
            ++tz_section_;
            tz_index_ = 0;
            context_->requestRedraw();
            return;
        }
        if (input.action == InputAction::Confirm) {
            pane_ = Pane::Detail;
            context_->requestRedraw();
        }
        return;
    }

    const int count = timeZoneCountInSection(tz_section_);
    if (input.action == InputAction::Up && tz_index_ > 0) {
        --tz_index_;
        context_->requestRedraw();
        return;
    }
    if (input.action == InputAction::Down && tz_index_ + 1 < count) {
        ++tz_index_;
        context_->requestRedraw();
        return;
    }
    if (input.action == InputAction::Confirm && count > 0) {
        context_->clock().setTimeZone(timeZoneIdInSection(tz_section_, tz_index_));
        editor_ = Editor::None;
        pane_ = Pane::Detail;
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
            wifi_section_ = WifiSection::Status;
            pane_ = Pane::Category;
            editor_index_ = 0;
            context_->requestRedraw();
            return;
        }
        if (isTimeZone()) {
            enterTimeZoneEditor();
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
        label = "Wi-Fi";
        value = wifi;
        return;
    }
    if (category_ == kTime) {
        label = "Time zone";
        value = zone;
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
    const int start = listWindowStart(category_, kCategoryCount, kCategoryVisible);
    const bool overflow = kCategoryCount > kCategoryVisible;
    if (overflow) {
        drawOverflowScrollbar(renderer.surface(), palette,
                              {left_x + kCategoryPaneWidth - kOuterPad - kScrollbarWidth,
                               outer_y + kOuterPad, kScrollbarWidth, outer_h - 2 * kOuterPad},
                              kCategoryCount, start, kCategoryVisible);
    }
    const int inner_w =
        kCategoryPaneWidth - 2 * kOuterPad - (overflow ? kScrollbarWidth + 2 : 0);
    const int inner_y0 = outer_y + kOuterPad;
    for (int i = 0; i < kCategoryVisible && start + i < kCategoryCount; ++i) {
        const int index = start + i;
        const Rect card{inner_x, inner_y0 + i * (kInnerCardHeight + kInnerCardGap), inner_w,
                        kInnerCardHeight};
        const bool selected = index == category_;
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
                                    kCategoryLabels[index]);
    }

    const int right_x = left_x + kCategoryPaneWidth + kPaneGap;
    const int right_w = layout::kWidth - layout::kChromeInset - right_x;
    const Rect right_outer{right_x, outer_y, right_w, outer_h};
    renderer.surface().fillRoundRect(right_outer, layout::kCardRadius, palette.card);

    const int detail_x = right_x + kOuterPad;
    const int detail_w = right_w - 2 * kOuterPad;
    int row_y = outer_y + kOuterPad;

    if (category_ == kBattery) {
        if (pane_ == Pane::Detail) {
            renderer.surface().drawRoundRect(right_outer, layout::kCardRadius, palette.accent);
        }
        Battery& battery = context_->battery();
        const BatteryReading reading = battery.current();
        char percent[8] = "--";
        if (reading.percent_valid) {
            std::snprintf(percent, sizeof(percent), "%u%%", static_cast<unsigned>(reading.percent));
        }
        const char* state = "--";
        if (reading.charging_valid) {
            state = reading.charging ? "Charging" : "Not charging";
        }
        char volts[12] = "--";
        if (reading.voltage_valid) {
            std::snprintf(volts, sizeof(volts), "%u.%02u V",
                          static_cast<unsigned>(reading.voltage_mv / 1000),
                          static_cast<unsigned>((reading.voltage_mv % 1000) / 10));
        }
        renderer.surface().drawText({detail_x + 4, centeredTextY(row_y, kRowBoxHeight)},
                                    {palette.primary_text, 1}, percent);
        const int state_x = detail_x + detail_w - 4 - font::textWidth(state, 1);
        renderer.surface().drawText({state_x, centeredTextY(row_y, kRowBoxHeight)},
                                    {palette.secondary_text, 1}, state);
        row_y += kRowBoxHeight + kInnerCardGap;
        renderer.surface().drawText({detail_x + 4, centeredTextY(row_y, kRowBoxHeight)},
                                    {palette.secondary_text, 1}, volts);
        row_y += kRowBoxHeight + kInnerCardGap;
        const int chart_bottom = outer_y + outer_h - kOuterPad;
        const Rect chart{detail_x, row_y, detail_w, chart_bottom - row_y};
        BatterySample history[Battery::kMaxSamples] = {};
        const int n = battery.sampleCount();
        for (int i = 0; i < n; ++i) {
            battery.sampleAt(i, history[i]);
        }
        drawBatteryHistory(renderer.surface(), palette, chart, history, n);
        const KeyHint hints[] = {{"Ent", "ok"}, {"Esc", "back"}};
        drawStandardFooter(renderer, hints, 2);
        renderer.endFrame();
        return;
    }

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
    const int left_x = layout::kChromeInset;
    const int outer_y = layout::kContentBoth.y + 2;
    const int outer_h = layout::kContentBoth.h - 4;
    const Rect left_outer{left_x, outer_y, kCategoryPaneWidth, outer_h};
    renderer.surface().fillRoundRect(left_outer, layout::kCardRadius, palette.card);

    const int inner_x = left_x + kOuterPad;
    const int inner_w = kCategoryPaneWidth - 2 * kOuterPad;
    const int inner_y0 = outer_y + kOuterPad;
    const bool left_focused = editor_ != Editor::Password && pane_ == Pane::Category;
    for (int i = 0; i < kWifiSectionCount; ++i) {
        const Rect card{inner_x, inner_y0 + i * (kInnerCardHeight + kInnerCardGap), inner_w,
                        kInnerCardHeight};
        const bool selected = i == static_cast<int>(wifi_section_);
        if (selected && left_focused) {
            renderer.surface().fillRoundRect(card, layout::kCardRadius, palette.accent);
        } else {
            renderer.surface().fillRoundRect(card, layout::kCardRadius, palette.canvas);
            if (selected) {
                renderer.surface().drawRoundRect(card, layout::kCardRadius, palette.accent);
            }
        }
        const Color text = selected && left_focused
                               ? palette.canvas
                               : (selected ? palette.primary_text : palette.secondary_text);
        renderer.surface().drawText({card.x + 4, centeredTextY(card.y, card.h)}, {text, 1},
                                    kWifiSectionLabels[i]);
    }

    const int right_x = left_x + kCategoryPaneWidth + kPaneGap;
    const int right_w = layout::kWidth - layout::kChromeInset - right_x;
    const Rect right_outer{right_x, outer_y, right_w, outer_h};
    renderer.surface().fillRoundRect(right_outer, layout::kCardRadius, palette.card);

    const int detail_x = right_x + kOuterPad;
    const int detail_w = right_w - 2 * kOuterPad;
    int row_y = outer_y + kOuterPad;
    const bool right_focused = editor_ == Editor::Password || pane_ == Pane::Detail;

    if (editor_ == Editor::Password) {
        const int text_pad = 4;
        const Rect ssid_label{detail_x, row_y, detail_w, kInnerCardHeight};
        renderer.surface().drawText({ssid_label.x + text_pad, centeredTextY(ssid_label.y, ssid_label.h)},
                                    {palette.secondary_text, 1}, "SSID");
        row_y += kInnerCardHeight + kInnerCardGap;
        const Rect ssid_card{detail_x, row_y, detail_w, kInnerCardHeight};
        renderer.surface().fillRoundRect(ssid_card, layout::kCardRadius, palette.canvas);
        renderer.surface().drawRoundRect(ssid_card, layout::kCardRadius, palette.secondary_text);
        char ssid_fit[33] = {};
        copyPrefixFitting(pending_ssid_, ssid_fit, sizeof(ssid_fit), ssid_card.w - 2 * text_pad);
        renderer.surface().drawText({ssid_card.x + text_pad, centeredTextY(ssid_card.y, ssid_card.h)},
                                    {palette.primary_text, 1}, ssid_fit);
        row_y += kInnerCardHeight + kInnerCardGap;

        char count[8] = {};
        std::snprintf(count, sizeof(count), "%u", static_cast<unsigned>(password_len_));
        const Rect pass_label{detail_x, row_y, detail_w, kInnerCardHeight};
        renderer.surface().drawText(
            {pass_label.x + text_pad, centeredTextY(pass_label.y, pass_label.h)},
            {palette.secondary_text, 1}, "Password");
        renderer.surface().drawText(
            {pass_label.x + pass_label.w - text_pad - font::textWidth(count, 1),
             centeredTextY(pass_label.y, pass_label.h)},
            {palette.secondary_text, 1}, count);
        row_y += kInnerCardHeight + kInnerCardGap;
        const Rect pass_card{detail_x, row_y, detail_w, kInnerCardHeight};
        renderer.surface().fillRoundRect(pass_card, layout::kCardRadius, palette.canvas);
        renderer.surface().drawRoundRect(pass_card, layout::kCardRadius, palette.accent);
        constexpr int kCursorW = 1;
        constexpr int kCursorGap = 1;
        const int mask_max_w = pass_card.w - 2 * text_pad - kCursorW - kCursorGap;
        const int visible = visibleMaskCount(password_len_, mask_max_w);
        char masked[64] = {};
        for (int i = 0; i < visible && i + 1 < static_cast<int>(sizeof(masked)); ++i) {
            masked[i] = '*';
        }
        const int text_y = centeredTextY(pass_card.y, pass_card.h);
        renderer.surface().drawText({pass_card.x + text_pad, text_y}, {palette.primary_text, 1},
                                    masked);
        const int cursor_x = pass_card.x + text_pad + font::textWidth(masked, 1) + kCursorGap;
        renderer.surface().fillRect({cursor_x, text_y, kCursorW, font::kGlyphHeight}, palette.accent);
        const KeyHint hints[] = {{"Ent", "join"}, {"Del", "bk"}, {"Esc", "back"}};
        drawStandardFooter(renderer, hints, 3);
        renderer.endFrame();
        return;
    }

    if (wifi_section_ == WifiSection::Status) {
        const int count = statusRowCount();
        char signal[24] = {};
        char ip[16] = {};
        if (network.state() == NetworkState::Connected) {
            std::snprintf(signal, sizeof(signal), "%d dBm", static_cast<int>(network.rssi()));
            network.stationIp(ip, sizeof(ip));
        }
        for (int i = 0; i < count; ++i) {
            const char* label = "State";
            const char* value = networkStateName();
            if (i == 1) {
                label = "SSID";
                value = network.connectedSsid();
            } else if (i == 2) {
                label = "Signal";
                value = signal;
            } else if (i == 3) {
                label = "IP";
                value = ip;
            } else if (i == 4) {
                label = "Disconnect";
                value = "";
            }
            const Rect row{detail_x, row_y, detail_w, kRowBoxHeight};
            drawEditorRow(renderer.surface(), palette, row, label, value,
                          right_focused && i == editor_index_);
            row_y += kRowBoxHeight + kInnerCardGap;
        }
        const KeyHint hints[] = {{"Ent", "ok"}, {"Esc", "back"}};
        drawStandardFooter(renderer, hints, 2);
        renderer.endFrame();
        return;
    }

    if (wifi_section_ == WifiSection::Saved) {
        const int profiles = network.profileCount();
        if (profiles == 0) {
            renderer.surface().drawText({detail_x + 4, row_y + 2}, {palette.secondary_text, 1},
                                        "No saved");
        } else {
            for (int i = 0; i < profiles; ++i) {
                const Rect row{detail_x, row_y, detail_w, kRowBoxHeight};
                SignalStrength strength = SignalStrength::None;
                const bool show_wifi = savedSignalStrength(network, i, strength);
                drawSsidIconRow(renderer.surface(), palette, row, network.profileSsid(i),
                                network.profileHasPassword(i), show_wifi, strength,
                                right_focused && i == editor_index_);
                row_y += kRowBoxHeight + kInnerCardGap;
            }
        }
        const KeyHint hints[] = {{"Ent", "ok"}, {"Del", "forget"}, {"Esc", "back"}};
        drawStandardFooter(renderer, hints, 3);
        renderer.endFrame();
        return;
    }

    const int hits = network.publicScanCount();
    const bool scanning = network.scanInProgress();
    const int count = scanRowCount();
    const int start = listWindowStart(editor_index_, count, kListVisible);
    if (count > kListVisible) {
        drawOverflowScrollbar(renderer.surface(), palette,
                              {right_outer.x + right_outer.w - kOuterPad - kScrollbarWidth,
                               outer_y + kOuterPad, kScrollbarWidth,
                               outer_h - 2 * kOuterPad},
                              count, start, kListVisible);
    }
    const int row_w =
        count > kListVisible ? detail_w - kScrollbarWidth - 2 : detail_w;
    for (int i = 0; i < kListVisible && start + i < count; ++i) {
        const int index = start + i;
        const Rect row{detail_x, row_y, row_w, kRowBoxHeight};
        if (!scanning && hits > 0) {
            WifiScanHit hit;
            if (network.publicScanAt(index, hit)) {
                drawSsidIconRow(renderer.surface(), palette, row, hit.ssid, hit.encrypted, true,
                                signalStrengthFromRssi(hit.rssi),
                                right_focused && index == editor_index_);
                row_y += kRowBoxHeight + kInnerCardGap;
                continue;
            }
        }
        drawEditorRow(renderer.surface(), palette, row, scanning ? "Scanning" : "No networks",
                      nullptr, false);
        row_y += kRowBoxHeight + kInnerCardGap;
    }
    const KeyHint hints[] = {{"Ent", "ok"}, {"Esc", "back"}};
    drawStandardFooter(renderer, hints, 2);
    renderer.endFrame();
}

void SettingsApp::drawTimeZoneEditor() {
    const theme::Palette palette = theme::paletteFor(context_->settings().theme(), accent());
    UiRenderer renderer(context_->display(), palette);
    renderer.beginFrame();
    renderer.clearAppCanvas();
    drawStandardHeader(*context_, renderer, name());

    const int left_x = layout::kChromeInset;
    const int outer_y = layout::kContentBoth.y + 2;
    const int outer_h = layout::kContentBoth.h - 4;
    const Rect left_outer{left_x, outer_y, kCategoryPaneWidth, outer_h};
    renderer.surface().fillRoundRect(left_outer, layout::kCardRadius, palette.card);

    const int inner_x = left_x + kOuterPad;
    const int inner_w = kCategoryPaneWidth - 2 * kOuterPad;
    const int inner_y0 = outer_y + kOuterPad;
    const bool left_focused = pane_ == Pane::Category;
    const int sections = timeZoneSectionCount();
    for (int i = 0; i < sections; ++i) {
        const Rect card{inner_x, inner_y0 + i * (kInnerCardHeight + kInnerCardGap), inner_w,
                        kInnerCardHeight};
        const bool selected = i == tz_section_;
        if (selected && left_focused) {
            renderer.surface().fillRoundRect(card, layout::kCardRadius, palette.accent);
        } else {
            renderer.surface().fillRoundRect(card, layout::kCardRadius, palette.canvas);
            if (selected) {
                renderer.surface().drawRoundRect(card, layout::kCardRadius, palette.accent);
            }
        }
        const Color text = selected && left_focused
                               ? palette.canvas
                               : (selected ? palette.primary_text : palette.secondary_text);
        renderer.surface().drawText({card.x + 4, centeredTextY(card.y, card.h)}, {text, 1},
                                    timeZoneSectionLabelAt(i));
    }

    const int right_x = left_x + kCategoryPaneWidth + kPaneGap;
    const int right_w = layout::kWidth - layout::kChromeInset - right_x;
    const Rect right_outer{right_x, outer_y, right_w, outer_h};
    renderer.surface().fillRoundRect(right_outer, layout::kCardRadius, palette.card);

    const int detail_x = right_x + kOuterPad;
    const int detail_w = right_w - 2 * kOuterPad;
    int row_y = outer_y + kOuterPad;
    const bool right_focused = pane_ == Pane::Detail;
    const int count = timeZoneCountInSection(tz_section_);
    const int start = listWindowStart(tz_index_, count, kListVisible);
    if (count > kListVisible) {
        drawOverflowScrollbar(renderer.surface(), palette,
                              {right_outer.x + right_outer.w - kOuterPad - kScrollbarWidth,
                               outer_y + kOuterPad, kScrollbarWidth, outer_h - 2 * kOuterPad},
                              count, start, kListVisible);
    }
    const int row_w = count > kListVisible ? detail_w - kScrollbarWidth - 2 : detail_w;
    for (int i = 0; i < kListVisible && start + i < count; ++i) {
        const int index = start + i;
        const Rect row{detail_x, row_y, row_w, kRowBoxHeight};
        char utc[12] = {};
        formatTimeZoneUtcLabel(timeZoneIdInSection(tz_section_, index), utc, sizeof(utc));
        drawEditorRow(renderer.surface(), palette, row, timeZoneIdInSection(tz_section_, index), utc,
                      right_focused && index == tz_index_);
        row_y += kRowBoxHeight + kInnerCardGap;
    }

    const KeyHint hints[] = {{"Ent", "ok"}, {"Esc", "back"}};
    drawStandardFooter(renderer, hints, 2);
    renderer.endFrame();
}

void SettingsApp::draw() {
    if (context_ == nullptr) {
        return;
    }
    if (editor_ == Editor::Wifi || editor_ == Editor::Password) {
        drawWifiEditor();
        return;
    }
    if (editor_ == Editor::TimeZone) {
        drawTimeZoneEditor();
        return;
    }
    drawSplitPane();
}

}  // namespace luma
