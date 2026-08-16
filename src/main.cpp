#include <Arduino.h>
#include <M5Cardputer.h>

#include "luma/luma.h"

luma::Luma runtime;

void setup() {
    Serial.begin(115200);

    auto config = M5.config();
    M5Cardputer.begin(config, true);

    runtime.begin();
}

void loop() {
    M5Cardputer.update();
    runtime.update();
}
