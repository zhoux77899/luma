#include "preview-menu.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <SDL.h>
#include <SDL_syswm.h>

namespace luma {
namespace {

constexpr UINT kCopyId = 40001;
constexpr UINT kSaveId = 40002;

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

void restoreClientSize(HWND hwnd, int client_w, int client_h) {
    RECT desired{0, 0, client_w, client_h};
    const DWORD style = static_cast<DWORD>(GetWindowLongPtrW(hwnd, GWL_STYLE));
    const DWORD ex_style = static_cast<DWORD>(GetWindowLongPtrW(hwnd, GWL_EXSTYLE));
    AdjustWindowRectEx(&desired, style, TRUE, ex_style);
    SetWindowPos(hwnd, nullptr, 0, 0, desired.right - desired.left, desired.bottom - desired.top,
                 SWP_NOMOVE | SWP_NOZORDER);
}

}  // namespace

void attachPreviewMenu(SDL_Window* window) {
    HWND hwnd = windowHandle(window);
    if (hwnd == nullptr) {
        return;
    }

    RECT client{};
    GetClientRect(hwnd, &client);
    const int client_w = client.right - client.left;
    const int client_h = client.bottom - client.top;

    HMENU menu = CreateMenu();
    HMENU file = CreatePopupMenu();
    AppendMenuW(file, MF_STRING, kCopyId, L"Copy Screenshot\tCtrl+Shift+C");
    AppendMenuW(file, MF_STRING, kSaveId, L"Save Screenshot...\tCtrl+Shift+S");
    AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(file), L"File");
    SetMenu(hwnd, menu);
    DrawMenuBar(hwnd);
    restoreClientSize(hwnd, client_w, client_h);

    SDL_EventState(SDL_SYSWMEVENT, SDL_ENABLE);
}

int previewMenuTopInset() { return 0; }

bool handlePreviewMenuEvent(const SDL_Event& event, PreviewCommand& command) {
    if (event.type != SDL_SYSWMEVENT || event.syswm.msg == nullptr) {
        return false;
    }
    if (event.syswm.msg->msg.win.msg != WM_COMMAND) {
        return false;
    }

    const int id = LOWORD(event.syswm.msg->msg.win.wParam);
    if (id == static_cast<int>(kCopyId)) {
        command = PreviewCommand::CopyScreenshot;
        return true;
    }
    if (id == static_cast<int>(kSaveId)) {
        command = PreviewCommand::SaveScreenshot;
        return true;
    }
    return false;
}

PreviewCommand takePreviewMenuCommand() { return PreviewCommand::None; }

void drawPreviewMenuOverlay(SDL_Renderer*, int, int) {}

}  // namespace luma
