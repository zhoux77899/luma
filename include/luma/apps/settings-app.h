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
    enum class Editor : int { None, Wifi, Password, TimeZone };
    enum class WifiSection : int { Status, Saved, Scan };

    void applyImmediate();
    void changeSelected(int delta);
    bool handleValueKey(const InputFrame& input);
    int detailCount() const;
    bool isBrightness() const;
    bool isVolume() const;
    bool isTheme() const;
    bool isAbout() const;
    bool isWifi() const;
    bool isTimeZone() const;
    void detailLabelValue(int index, const char*& label, const char*& value, char* brightness,
                          char* volume, const char* theme_label, char* wifi, char* zone) const;
    void updateSplitPane(const InputFrame& input);
    void updateWifiEditor(const InputFrame& input);
    void updatePasswordEditor(const InputFrame& input);
    void updateTimeZoneEditor(const InputFrame& input);
    void drawSplitPane();
    void drawWifiEditor();
    void drawTimeZoneEditor();
    void enterWifiStatusDetail();
    void enterTimeZoneEditor();
    int wifiSectionCount() const;
    int wifiDetailCount() const;
    int statusRowCount() const;
    int scanRowCount() const;
    const char* networkStateLabel() const;
    const char* networkStateName() const;

    AppContext* context_ = nullptr;
    Pane pane_ = Pane::Category;
    Editor editor_ = Editor::None;
    WifiSection wifi_section_ = WifiSection::Status;
    int category_ = 0;
    int detail_ = 0;
    int editor_index_ = 0;
    int tz_section_ = 0;
    int tz_index_ = 0;
    char pending_ssid_[33] = {};
    char password_[64] = {};
    uint8_t password_len_ = 0;
};

}  // namespace luma
