#pragma once

#include <cstdint>

namespace luma {

enum class InputAction : uint8_t {
    None,
    Up,
    Down,
    Left,
    Right,
    Confirm,
    Back,
    Delete,
    PagePrevious,
    PageNext,
};

struct InputFrame {
    InputAction action = InputAction::None;
    char text[32] = {};
    uint8_t textLength = 0;
    bool pressed = false;
    bool repeated = false;
};

}  // namespace luma
