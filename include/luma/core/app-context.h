#pragma once

namespace luma {

class Clock;
class DisplaySurface;
class Settings;
class Storage;

class AppContext {
public:
    AppContext(DisplaySurface& display, Settings& settings, Storage& storage, Clock& clock);

    DisplaySurface& display();
    Settings& settings();
    Storage& storage();
    Clock& clock();

    void requestRedraw();
    bool takeRedrawRequest();
    void requestEnter(const char* id);
    const char* takeEnterRequest();

private:
    DisplaySurface& display_;
    Settings& settings_;
    Storage& storage_;
    Clock& clock_;
    bool redraw_requested_ = false;
    const char* enter_id_ = nullptr;
};

}  // namespace luma
