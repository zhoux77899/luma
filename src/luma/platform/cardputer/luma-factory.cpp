#include "cardputer-clock.h"
#include "cardputer-display-adapter.h"
#include "cardputer-input-adapter.h"
#include "serial-diagnostics.h"

#include "luma/core/in-memory-storage.h"
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

InMemoryStorage& storage() {
    static InMemoryStorage instance;
    return instance;
}

Settings& settings() {
    static Settings instance;
    return instance;
}

}  // namespace

Luma::Luma() : Luma(display(), input(), clock(), storage(), settings(), diagnostics()) {}

}  // namespace luma
