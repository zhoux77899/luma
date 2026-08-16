#include "cardputer-input-adapter.h"

#include <M5Cardputer.h>

namespace luma {

CardputerInputAdapter::CardputerInputAdapter(Diagnostics& diagnostics) : diagnostics_(diagnostics) {}

bool CardputerInputAdapter::poll(InputFrame& frame) {
    frame = InputFrame{};

    if (M5Cardputer.BtnA.wasPressed()) {
        frame.action = InputAction::Confirm;
        frame.pressed = true;
        return true;
    }

    if (!M5Cardputer.Keyboard.isChange() || !M5Cardputer.Keyboard.isPressed()) {
        return false;
    }

    const auto state = M5Cardputer.Keyboard.keysState();
    if (state.enter) {
        frame.action = InputAction::Confirm;
    } else if (state.esc) {
        frame.action = InputAction::Back;
    } else if (state.del || state.backspace) {
        frame.action = InputAction::Delete;
    } else if (state.up) {
        frame.action = InputAction::Up;
    } else if (state.down) {
        frame.action = InputAction::Down;
    } else if (state.left) {
        frame.action = InputAction::Left;
    } else if (state.right) {
        frame.action = InputAction::Right;
    }

    constexpr uint8_t kMaxText = 31;
    uint8_t length = 0;
    if (state.word.size() > kMaxText) {
        diagnostics_.emit("ERROR", "input truncated");
    }
    for (const char character : state.word) {
        if (length >= kMaxText) {
            break;
        }
        frame.text[length++] = character;
    }
    frame.textLength = length;
    frame.text[length] = '\0';
    frame.pressed = true;
    return true;
}

}  // namespace luma
