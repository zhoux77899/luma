#include "sdl-input-adapter.h"

#include "preview-host.h"

#include <SDL.h>

namespace luma {
namespace {

bool isHostShortcut(const SDL_Event& event, PreviewCommand& command) {
    if (event.type != SDL_KEYDOWN || event.key.repeat != 0) {
        return false;
    }
    const SDL_Keymod mods = SDL_GetModState();
    const bool host = (mods & (KMOD_CTRL | KMOD_GUI)) != 0;
    const bool shift = (mods & KMOD_SHIFT) != 0;
    if (!host || !shift) {
        return false;
    }
    if (event.key.keysym.sym == SDLK_c) {
        command = PreviewCommand::CopyScreenshot;
        return true;
    }
    if (event.key.keysym.sym == SDLK_s) {
        command = PreviewCommand::SaveScreenshot;
        return true;
    }
    return false;
}

InputAction actionForKey(SDL_Keycode key) {
    switch (key) {
        case SDLK_UP:
            return InputAction::Up;
        case SDLK_DOWN:
            return InputAction::Down;
        case SDLK_LEFT:
            return InputAction::Left;
        case SDLK_RIGHT:
            return InputAction::Right;
        case SDLK_RETURN:
        case SDLK_KP_ENTER:
            return InputAction::Confirm;
        case SDLK_ESCAPE:
            return InputAction::Back;
        case SDLK_BACKSPACE:
        case SDLK_DELETE:
            return InputAction::Delete;
        case SDLK_PAGEUP:
            return InputAction::PagePrevious;
        case SDLK_PAGEDOWN:
            return InputAction::PageNext;
        default:
            return InputAction::None;
    }
}

}  // namespace

void SdlInputAdapter::setHost(PreviewHost* host) { host_ = host; }

void SdlInputAdapter::pump() {
    SDL_Event event;
    while (SDL_PollEvent(&event) != 0) {
        if (event.type == SDL_QUIT) {
            quit_ = true;
            continue;
        }

        if (host_ != nullptr && host_->handleEvent(event)) {
            continue;
        }

        PreviewCommand host_command = PreviewCommand::None;
        if (isHostShortcut(event, host_command)) {
            command_ = host_command;
            continue;
        }

        if (event.type == SDL_KEYDOWN) {
            InputFrame frame;
            frame.action = actionForKey(event.key.keysym.sym);
            frame.pressed = true;
            frame.repeated = event.key.repeat != 0;
            if (frame.action != InputAction::None) {
                queue_.push_back(frame);
            }
            continue;
        }

        if (event.type == SDL_TEXTINPUT) {
            InputFrame frame;
            frame.pressed = true;
            for (int i = 0; event.text.text[i] != '\0' && frame.textLength < 31; ++i) {
                const char character = event.text.text[i];
                if (character < 32 || character > 126) {
                    continue;
                }
                frame.text[frame.textLength++] = character;
            }
            frame.text[frame.textLength] = '\0';
            if (frame.textLength > 0) {
                queue_.push_back(frame);
            }
        }
    }
}

bool SdlInputAdapter::poll(InputFrame& frame) {
    pump();
    if (queue_.empty()) {
        frame = InputFrame{};
        return false;
    }
    frame = queue_.front();
    queue_.erase(queue_.begin());
    return true;
}

bool SdlInputAdapter::quitRequested() const { return quit_; }

PreviewCommand SdlInputAdapter::takeCommand() {
    const PreviewCommand command = command_;
    command_ = PreviewCommand::None;
    return command;
}

}  // namespace luma
