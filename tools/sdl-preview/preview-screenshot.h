#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace luma {

void scaleNearestArgb(const uint32_t* src, int src_w, int src_h, int scale, std::vector<uint32_t>& dest);

bool argbToRgba(const uint32_t* argb, int width, int height, std::vector<unsigned char>& rgba);

bool encodePngRgba(const unsigned char* rgba, int width, int height, const char* path, std::string& error);

bool encodePngArgb(const uint32_t* argb, int width, int height, const char* path, std::string& error);

bool encodePngArgbToMemory(const uint32_t* argb, int width, int height, std::vector<unsigned char>& png,
                           std::string& error);

std::string previewScreenshotDefaultDirectory();
std::string previewScreenshotDefaultFilename();
bool ensureDirectory(const std::string& path, std::string& error);

}  // namespace luma
