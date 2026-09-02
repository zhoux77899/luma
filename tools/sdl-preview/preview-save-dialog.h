#pragma once

#include <string>

struct SDL_Window;

namespace luma {

enum class SaveDialogResult {
    Saved,
    Cancelled,
    Failed,
};

SaveDialogResult showSavePngDialog(SDL_Window* window, const std::string& directory, const std::string& filename,
                                   std::string& path, std::string& error);

}  // namespace luma
