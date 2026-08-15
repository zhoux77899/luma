#include <Arduino.h>
#include <M5Cardputer.h>

namespace {

void drawBootScreen() {
    M5Cardputer.Display.fillScreen(BLACK);
    M5Cardputer.Display.setRotation(1);
    M5Cardputer.Display.setTextSize(2);
    M5Cardputer.Display.setCursor(10, 10);
    M5Cardputer.Display.println("Luma / Cardputer ADV");
    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.println();
    M5Cardputer.Display.println("Keyboard and serial monitor ready.");
}

void handleKeyboard() {
    if (!M5Cardputer.Keyboard.isChange() ||
        !M5Cardputer.Keyboard.isPressed()) {
        return;
    }

    const auto state = M5Cardputer.Keyboard.keysState();

    for (const auto character : state.word) {
        M5Cardputer.Display.print(character);
        Serial.printf("[KEY] %c\n", character);
    }

    if (state.enter) {
        M5Cardputer.Display.println();
        Serial.println("[KEY] ENTER");
    }

    if (state.del) {
        Serial.println("[KEY] BACKSPACE");
    }
}

}  // namespace

void setup() {
    Serial.begin(115200);

    auto config = M5.config();
    M5Cardputer.begin(config, true);
    drawBootScreen();

    Serial.println("[BOOT] Luma Cardputer ADV started");
}

void loop() {
    M5Cardputer.update();
    handleKeyboard();
    delay(10);
}
