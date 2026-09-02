#pragma once

#include "preview-command.h"

namespace luma {

class SdlDisplayAdapter;

void runPreviewCommand(PreviewCommand command, SdlDisplayAdapter& display);

}  // namespace luma
