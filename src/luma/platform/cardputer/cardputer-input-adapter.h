#pragma once

#include "luma/core/diagnostics.h"
#include "luma/core/input-source.h"

namespace luma {

class CardputerInputAdapter : public InputSource {
public:
    explicit CardputerInputAdapter(Diagnostics& diagnostics);
    bool poll(InputFrame& frame) override;

private:
    Diagnostics& diagnostics_;
};

}  // namespace luma
