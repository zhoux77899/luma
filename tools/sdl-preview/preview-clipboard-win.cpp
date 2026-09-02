#include "preview-clipboard.h"

#include "preview-screenshot.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <SDL.h>
#include <SDL_syswm.h>

#include <cstring>
#include <vector>

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

bool copyDib(const uint32_t* argb, int width, int height) {
    const size_t header_size = sizeof(BITMAPINFOHEADER);
    const size_t pixel_bytes = static_cast<size_t>(width) * static_cast<size_t>(height) * 4u;
    HGLOBAL handle = GlobalAlloc(GMEM_MOVEABLE, header_size + pixel_bytes);
    if (handle == nullptr) {
        return false;
    }

    auto* memory = static_cast<unsigned char*>(GlobalLock(handle));
    if (memory == nullptr) {
        GlobalFree(handle);
        return false;
    }

    auto* header = reinterpret_cast<BITMAPINFOHEADER*>(memory);
    std::memset(header, 0, sizeof(*header));
    header->biSize = sizeof(BITMAPINFOHEADER);
    header->biWidth = width;
    header->biHeight = height;
    header->biPlanes = 1;
    header->biBitCount = 32;
    header->biCompression = BI_RGB;

    unsigned char* pixels = memory + header_size;
    for (int y = 0; y < height; ++y) {
        const uint32_t* src_row = argb + (height - 1 - y) * width;
        unsigned char* dest_row = pixels + static_cast<size_t>(y) * static_cast<size_t>(width) * 4u;
        for (int x = 0; x < width; ++x) {
            const uint32_t pixel = src_row[x];
            dest_row[x * 4 + 0] = static_cast<unsigned char>(pixel & 0xFFu);
            dest_row[x * 4 + 1] = static_cast<unsigned char>((pixel >> 8) & 0xFFu);
            dest_row[x * 4 + 2] = static_cast<unsigned char>((pixel >> 16) & 0xFFu);
            dest_row[x * 4 + 3] = 0xFF;
        }
    }

    GlobalUnlock(handle);
    if (SetClipboardData(CF_DIB, handle) == nullptr) {
        GlobalFree(handle);
        return false;
    }
    return true;
}

bool copyPng(const uint32_t* argb, int width, int height, std::string& error) {
    std::vector<unsigned char> png;
    if (!encodePngArgbToMemory(argb, width, height, png, error)) {
        return false;
    }

    const UINT format = RegisterClipboardFormatA("PNG");
    if (format == 0) {
        return true;
    }

    HGLOBAL handle = GlobalAlloc(GMEM_MOVEABLE, png.size());
    if (handle == nullptr) {
        error = "Failed to copy Preview screenshot";
        return false;
    }
    void* memory = GlobalLock(handle);
    if (memory == nullptr) {
        GlobalFree(handle);
        error = "Failed to copy Preview screenshot";
        return false;
    }
    std::memcpy(memory, png.data(), png.size());
    GlobalUnlock(handle);
    if (SetClipboardData(format, handle) == nullptr) {
        GlobalFree(handle);
        error = "Failed to copy Preview screenshot";
        return false;
    }
    return true;
}

}  // namespace

bool copyPreviewScreenshot(SDL_Window* window, const uint32_t* argb, int width, int height, std::string& error) {
    const HWND hwnd = windowHandle(window);
    if (!OpenClipboard(hwnd)) {
        error = "Failed to open the clipboard";
        return false;
    }
    EmptyClipboard();
    const bool dib = copyDib(argb, width, height);
    if (dib) {
        std::string png_error;
        copyPng(argb, width, height, png_error);
    }
    CloseClipboard();
    if (!dib) {
        error = "Failed to copy Preview screenshot";
        return false;
    }
    return true;
}

void pumpPreviewClipboard(SDL_Window*) {}

}  // namespace luma
