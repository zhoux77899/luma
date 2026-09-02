#include "preview-screenshot.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBI_WRITE_NO_STDIO
#include "vendor/stb_image_write.h"

#include <cstdio>
#include <ctime>
#include <filesystem>
#include <fstream>

#ifndef LUMA_SOURCE_ROOT
#define LUMA_SOURCE_ROOT "."
#endif

namespace luma {
namespace {

struct PngMemory {
    std::vector<unsigned char>* out = nullptr;
};

void writePngChunk(void* context, void* data, int size) {
    auto* memory = static_cast<PngMemory*>(context);
    const auto* bytes = static_cast<const unsigned char*>(data);
    memory->out->insert(memory->out->end(), bytes, bytes + size);
}

}  // namespace

void scaleNearestArgb(const uint32_t* src, int src_w, int src_h, int scale, std::vector<uint32_t>& dest) {
    dest.clear();
    if (src == nullptr || src_w <= 0 || src_h <= 0 || scale < 1) {
        return;
    }

    const int dest_w = src_w * scale;
    const int dest_h = src_h * scale;
    dest.resize(static_cast<size_t>(dest_w * dest_h));
    for (int y = 0; y < dest_h; ++y) {
        const int src_y = y / scale;
        for (int x = 0; x < dest_w; ++x) {
            const int src_x = x / scale;
            dest[static_cast<size_t>(y * dest_w + x)] = src[src_y * src_w + src_x];
        }
    }
}

bool argbToRgba(const uint32_t* argb, int width, int height, std::vector<unsigned char>& rgba) {
    rgba.clear();
    if (argb == nullptr || width <= 0 || height <= 0) {
        return false;
    }

    rgba.resize(static_cast<size_t>(width * height * 4));
    for (int i = 0; i < width * height; ++i) {
        const uint32_t pixel = argb[i];
        rgba[static_cast<size_t>(i) * 4 + 0] = static_cast<unsigned char>((pixel >> 16) & 0xFFu);
        rgba[static_cast<size_t>(i) * 4 + 1] = static_cast<unsigned char>((pixel >> 8) & 0xFFu);
        rgba[static_cast<size_t>(i) * 4 + 2] = static_cast<unsigned char>(pixel & 0xFFu);
        rgba[static_cast<size_t>(i) * 4 + 3] = 0xFF;
    }
    return true;
}

bool encodePngRgba(const unsigned char* rgba, int width, int height, const char* path, std::string& error) {
    std::vector<unsigned char> png;
    PngMemory memory{&png};
    if (stbi_write_png_to_func(writePngChunk, &memory, width, height, 4, rgba, width * 4) == 0) {
        error = "Failed to encode Preview screenshot PNG";
        return false;
    }

    std::ofstream out(path, std::ios::binary);
    if (!out) {
        error = "Failed to write Preview screenshot PNG";
        return false;
    }
    out.write(reinterpret_cast<const char*>(png.data()), static_cast<std::streamsize>(png.size()));
    if (!out) {
        error = "Failed to write Preview screenshot PNG";
        return false;
    }
    return true;
}

bool encodePngArgb(const uint32_t* argb, int width, int height, const char* path, std::string& error) {
    std::vector<unsigned char> rgba;
    if (!argbToRgba(argb, width, height, rgba)) {
        error = "Preview screenshot has no pixels";
        return false;
    }
    return encodePngRgba(rgba.data(), width, height, path, error);
}

bool encodePngArgbToMemory(const uint32_t* argb, int width, int height, std::vector<unsigned char>& png,
                           std::string& error) {
    png.clear();
    std::vector<unsigned char> rgba;
    if (!argbToRgba(argb, width, height, rgba)) {
        error = "Preview screenshot has no pixels";
        return false;
    }

    PngMemory memory{&png};
    if (stbi_write_png_to_func(writePngChunk, &memory, width, height, 4, rgba.data(), width * 4) == 0) {
        error = "Failed to encode Preview screenshot PNG";
        return false;
    }
    return true;
}

std::string previewScreenshotDefaultDirectory() {
    return std::string(LUMA_SOURCE_ROOT) + "/docs/user-manual/assets";
}

std::string previewScreenshotDefaultFilename() {
    std::time_t now = std::time(nullptr);
    std::tm local{};
#if defined(_WIN32)
    localtime_s(&local, &now);
#else
    localtime_r(&now, &local);
#endif
    char name[40];
    std::snprintf(name, sizeof(name), "luma-preview-%04d%02d%02d-%02d%02d%02d.png", local.tm_year + 1900,
                  local.tm_mon + 1, local.tm_mday, local.tm_hour, local.tm_min, local.tm_sec);
    return name;
}

bool ensureDirectory(const std::string& path, std::string& error) {
    std::error_code ec;
    std::filesystem::create_directories(path, ec);
    if (ec) {
        error = "Failed to create " + path;
        return false;
    }
    return true;
}

}  // namespace luma
