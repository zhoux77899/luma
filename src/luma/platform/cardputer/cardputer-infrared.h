#pragma once

#include "luma/core/diagnostics.h"
#include "luma/core/infrared.h"

namespace luma {

class CardputerInfrared : public Infrared {
public:
    explicit CardputerInfrared(Diagnostics& diagnostics);

    void begin() override;
    bool transmit(const Frame& frame) override;

private:
    Diagnostics& diagnostics_;
    bool begun_ = false;
};

}  // namespace luma
