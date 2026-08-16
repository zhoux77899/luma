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

private:
    DisplaySurface& display_;
    Settings& settings_;
    Storage& storage_;
    Clock& clock_;
    bool redraw_requested_ = false;
};

}  // namespace luma
