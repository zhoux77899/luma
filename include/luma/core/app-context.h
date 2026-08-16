#pragma once

namespace luma {

class Clock;
class Diagnostics;
class DisplaySurface;
class Settings;
class Storage;

class AppContext {
public:
    AppContext(DisplaySurface& display, Settings& settings, Storage& storage, Clock& clock,
               Diagnostics& diagnostics);

    DisplaySurface& display();
    Settings& settings();
    Storage& storage();
    Clock& clock();
    Diagnostics& diagnostics();

    void requestRedraw();
    bool takeRedrawRequest();
    void requestEnter(const char* id);
    const char* takeEnterRequest();
    void requestUiSound();
    bool takeUiSound();

private:
    DisplaySurface& display_;
    Settings& settings_;
    Storage& storage_;
    Clock& clock_;
    Diagnostics& diagnostics_;
    bool redraw_requested_ = false;
    const char* enter_id_ = nullptr;
    bool ui_sound_requested_ = false;
};

}  // namespace luma
