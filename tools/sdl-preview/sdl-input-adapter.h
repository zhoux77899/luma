#pragma once

#include "luma/core/input-source.h"
#include "preview-command.h"

#include <vector>

namespace luma {

class PreviewHost;

class SdlInputAdapter : public InputSource {
public:
    void setHost(PreviewHost* host);
    bool poll(InputFrame& frame) override;
    bool quitRequested() const;
    PreviewCommand takeCommand();

private:
    void pump();

    PreviewHost* host_ = nullptr;
    std::vector<InputFrame> queue_;
    PreviewCommand command_ = PreviewCommand::None;
    bool quit_ = false;
};

}  // namespace luma
