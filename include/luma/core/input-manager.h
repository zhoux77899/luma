#pragma once

#include "luma/core/input-frame.h"
#include "luma/core/input-source.h"

namespace luma {

class Diagnostics;

class InputManager {
public:
    InputManager(InputSource& source, Diagnostics& diagnostics);

    bool poll(InputFrame& frame);

private:
    void logKey(const InputFrame& frame);

    InputSource& source_;
    Diagnostics& diagnostics_;
};

}  // namespace luma
