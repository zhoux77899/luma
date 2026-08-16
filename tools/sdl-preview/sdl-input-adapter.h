#pragma once

#include "luma/core/input-source.h"

#include <vector>

namespace luma {

class SdlInputAdapter : public InputSource {
public:
    bool poll(InputFrame& frame) override;
    bool quitRequested() const;

private:
    void pump();

    std::vector<InputFrame> queue_;
    bool quit_ = false;
};

}  // namespace luma
