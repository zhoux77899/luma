#pragma once

#include "preview-command.h"

struct SDL_Window;
union SDL_Event;

namespace luma {

class PreviewHost {
public:
    void attach(SDL_Window* window);
    bool handleEvent(const SDL_Event& event);
    PreviewCommand takeCommand();
    void pump();

private:
    SDL_Window* window_ = nullptr;
    PreviewCommand command_ = PreviewCommand::None;
};

}  // namespace luma
