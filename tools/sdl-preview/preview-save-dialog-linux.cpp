#include "preview-save-dialog.h"

#include <array>
#include <cstdio>
#include <cstdlib>
#include <string>

namespace luma {
namespace {

std::string shellQuote(const std::string& value) {
    std::string quoted = "'";
    for (char character : value) {
        if (character == '\'') {
            quoted += "'\\''";
        } else {
            quoted += character;
        }
    }
    quoted += "'";
    return quoted;
}

bool commandExists(const char* name) {
    std::string command = "command -v ";
    command += name;
    command += " >/dev/null 2>&1";
    return std::system(command.c_str()) == 0;
}

bool runDialog(const std::string& command, std::string& path, int& status) {
    FILE* pipe = popen(command.c_str(), "r");
    if (pipe == nullptr) {
        status = -1;
        return false;
    }
    std::string output;
    std::array<char, 512> buffer{};
    while (std::fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr) {
        output += buffer.data();
    }
    status = pclose(pipe);
    while (!output.empty() && (output.back() == '\n' || output.back() == '\r')) {
        output.pop_back();
    }
    path = output;
    return !path.empty();
}

}  // namespace

SaveDialogResult showSavePngDialog(SDL_Window*, const std::string& directory, const std::string& filename,
                                   std::string& path, std::string& error) {
    std::string suggested = directory;
    if (!suggested.empty() && suggested.back() != '/') {
        suggested += '/';
    }
    suggested += filename;

    int status = 0;
    if (commandExists("zenity")) {
        if (runDialog("zenity --file-selection --save --confirm-overwrite --filename=" + shellQuote(suggested) +
                          " --title='Save Screenshot' 2>/dev/null",
                      path, status)) {
            return SaveDialogResult::Saved;
        }
        return status == 0 ? SaveDialogResult::Failed : SaveDialogResult::Cancelled;
    }

    if (commandExists("kdialog")) {
        if (runDialog("kdialog --getsavefilename " + shellQuote(suggested) + " 'PNG image (*.png)' 2>/dev/null", path,
                      status)) {
            return SaveDialogResult::Saved;
        }
        return status == 0 ? SaveDialogResult::Failed : SaveDialogResult::Cancelled;
    }

    error = "Save As needs zenity or kdialog";
    return SaveDialogResult::Failed;
}

}  // namespace luma
