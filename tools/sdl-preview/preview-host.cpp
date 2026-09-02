#include "preview-host.h"

#include "preview-clipboard.h"
#include "preview-menu.h"

#include <SDL.h>

namespace luma {

void PreviewHost::attach(SDL_Window* window) {
    window_ = window;
    attachPreviewMenu(window_);
}

bool PreviewHost::handleEvent(const SDL_Event& event) {
    PreviewCommand command = PreviewCommand::None;
    if (handlePreviewMenuEvent(event, command)) {
        if (command != PreviewCommand::None) {
            command_ = command;
        }
        return true;
    }
    return false;
}

PreviewCommand PreviewHost::takeCommand() {
    if (command_ == PreviewCommand::None) {
        command_ = takePreviewMenuCommand();
    }
    const PreviewCommand command = command_;
    command_ = PreviewCommand::None;
    return command;
}

void PreviewHost::pump() { pumpPreviewClipboard(window_); }

}  // namespace luma
