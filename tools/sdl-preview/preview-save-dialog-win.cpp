#include "preview-save-dialog.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <commdlg.h>

#include <SDL.h>
#include <SDL_syswm.h>

namespace luma {
namespace {

HWND windowHandle(SDL_Window* window) {
    if (window == nullptr) {
        return nullptr;
    }
    SDL_SysWMinfo info;
    SDL_VERSION(&info.version);
    if (SDL_GetWindowWMInfo(window, &info) == SDL_FALSE) {
        return nullptr;
    }
    return info.info.win.window;
}

std::wstring toWide(const std::string& text) {
    if (text.empty()) {
        return L"";
    }
    const int count = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, nullptr, 0);
    if (count <= 0) {
        return L"";
    }
    std::wstring wide(static_cast<size_t>(count - 1), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, wide.data(), count);
    return wide;
}

std::string toUtf8(const wchar_t* text) {
    if (text == nullptr || text[0] == L'\0') {
        return "";
    }
    const int count = WideCharToMultiByte(CP_UTF8, 0, text, -1, nullptr, 0, nullptr, nullptr);
    if (count <= 0) {
        return "";
    }
    std::string utf8(static_cast<size_t>(count - 1), '\0');
    WideCharToMultiByte(CP_UTF8, 0, text, -1, utf8.data(), count, nullptr, nullptr);
    return utf8;
}

}  // namespace

SaveDialogResult showSavePngDialog(SDL_Window* window, const std::string& directory, const std::string& filename,
                                   std::string& path, std::string& error) {
    std::wstring file = toWide(filename);
    file.resize(32768);
    const std::wstring dir = toWide(directory);

    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = windowHandle(window);
    ofn.lpstrFilter = L"PNG image\0*.png\0\0";
    ofn.lpstrFile = file.data();
    ofn.nMaxFile = static_cast<DWORD>(file.size());
    ofn.lpstrInitialDir = dir.empty() ? nullptr : dir.c_str();
    ofn.lpstrDefExt = L"png";
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;

    if (GetSaveFileNameW(&ofn) == TRUE) {
        path = toUtf8(file.c_str());
        if (path.empty()) {
            error = "Failed to read the save path";
            return SaveDialogResult::Failed;
        }
        return SaveDialogResult::Saved;
    }
    if (CommDlgExtendedError() == 0) {
        return SaveDialogResult::Cancelled;
    }
    error = "Failed to open Save As";
    return SaveDialogResult::Failed;
}

}  // namespace luma
