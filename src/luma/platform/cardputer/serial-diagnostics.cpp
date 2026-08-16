#include "serial-diagnostics.h"

#include <Arduino.h>

namespace luma {

void SerialDiagnostics::emit(const char* prefix, const char* message) {
    Serial.printf("[%s] %s\n", prefix, message);
}

}  // namespace luma
