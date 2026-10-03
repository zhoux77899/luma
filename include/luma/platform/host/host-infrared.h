#pragma once

#include "luma/core/diagnostics.h"
#include "luma/core/infrared.h"

namespace luma {

class HostInfrared : public Infrared {
public:
    explicit HostInfrared(Diagnostics& diagnostics);

    bool transmit(const Frame& frame) override;

private:
    Diagnostics& diagnostics_;
};

}  // namespace luma
