#include "luma/core/input-manager.h"

#include "luma/core/diagnostics.h"

namespace luma {
namespace {

void emitKey(Diagnostics& diagnostics, const char* message) {
    diagnostics.emit("KEY", message);
}

}  // namespace

InputManager::InputManager(InputSource& source, Diagnostics& diagnostics)
    : source_(source), diagnostics_(diagnostics) {}

bool InputManager::poll(InputFrame& frame) {
    frame = InputFrame{};
    if (!source_.poll(frame)) {
        frame = InputFrame{};
        return false;
    }

    if (frame.textLength > 31) {
        frame.textLength = 31;
        frame.text[31] = '\0';
        diagnostics_.emit("ERROR", "input truncated");
    } else if (frame.textLength < 32) {
        frame.text[frame.textLength] = '\0';
    }

    logKey(frame);
    return true;
}

void InputManager::logKey(const InputFrame& frame) {
    for (uint8_t i = 0; i < frame.textLength; ++i) {
        char message[2] = {frame.text[i], '\0'};
        emitKey(diagnostics_, message);
    }

    switch (frame.action) {
        case InputAction::Confirm:
            emitKey(diagnostics_, "ENTER");
            break;
        case InputAction::Back:
            emitKey(diagnostics_, "ESC");
            break;
        case InputAction::Delete:
            emitKey(diagnostics_, "BACKSPACE");
            break;
        default:
            break;
    }
}

}  // namespace luma
