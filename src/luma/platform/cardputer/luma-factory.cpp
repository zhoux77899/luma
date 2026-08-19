#include "cardputer-audio-adapter.h"
#include "cardputer-clock.h"
#include "cardputer-display-adapter.h"
#include "cardputer-input-adapter.h"
#include "cardputer-wifi-radio.h"
#include "nvs-littlefs-storage.h"
#include "serial-diagnostics.h"

#include "luma/core/network.h"
#include "luma/core/settings.h"
#include "luma/luma.h"

namespace luma {
namespace {

SerialDiagnostics& diagnostics() {
    static SerialDiagnostics instance;
    return instance;
}

CardputerDisplayAdapter& display() {
    static CardputerDisplayAdapter instance;
    return instance;
}

CardputerInputAdapter& input() {
    static CardputerInputAdapter instance(diagnostics());
    return instance;
}

CardputerClock& clock() {
    static CardputerClock instance;
    return instance;
}

NvsLittleFsStorage& storage() {
    static NvsLittleFsStorage instance(diagnostics());
    return instance;
}

CardputerAudioAdapter& audio() {
    static CardputerAudioAdapter instance;
    return instance;
}

Settings& settings() {
    static Settings instance;
    return instance;
}

CardputerWifiRadio& radio() {
    static CardputerWifiRadio instance;
    return instance;
}

Network& network() {
    static Network instance;
    return instance;
}

}  // namespace

Luma::Luma() : Luma(display(), input(), clock(), storage(), settings(), diagnostics(), audio(),
                    network()) {
    network().attach(radio(), storage(), diagnostics(), clock());
}

}  // namespace luma
