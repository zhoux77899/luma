#pragma once

#include "luma/core/input-frame.h"

namespace luma {

class InputSource {
public:
    virtual ~InputSource() = default;
    virtual bool poll(InputFrame& frame) = 0;
};

}  // namespace luma
