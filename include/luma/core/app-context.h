#pragma once

namespace luma {

class Clock;
class Diagnostics;
class DisplaySurface;
class Network;
class Settings;
class Storage;

class AppContext {
public:
    AppContext(DisplaySurface& display, Settings& settings, Storage& storage, Clock& clock,
               Diagnostics& diagnostics, Network& network);

    DisplaySurface& display();
    Settings& settings();
    Storage& storage();
    Clock& clock();
    Diagnostics& diagnostics();
    Network& network();

    void requestRedraw();
    bool takeRedrawRequest();
    void requestEnter(const char* id);
    const char* takeEnterRequest();
    void requestUiSound();
    bool takeUiSound();
    void consumeBack();
    bool takeBackConsumed();

private:
    DisplaySurface& display_;
    Settings& settings_;
    Storage& storage_;
    Clock& clock_;
    Diagnostics& diagnostics_;
    Network& network_;
    bool redraw_requested_ = false;
    const char* enter_id_ = nullptr;
    bool ui_sound_requested_ = false;
    bool back_consumed_ = false;
};

}  // namespace luma
