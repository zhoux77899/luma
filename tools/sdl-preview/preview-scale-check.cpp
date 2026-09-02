#include "preview-screenshot.h"

#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace {

bool expectPixel(const std::vector<uint32_t>& dest, int width, int x, int y, uint32_t color) {
    return dest[static_cast<size_t>(y * width + x)] == color;
}

}  // namespace

int main() {
    const uint32_t src[] = {0xFFFF0000u, 0xFF00FF00u, 0xFF0000FFu, 0xFFFFFFFFu};
    std::vector<uint32_t> dest;
    luma::scaleNearestArgb(src, 2, 2, 2, dest);
    if (dest.size() != 16) {
        std::fprintf(stderr, "scaled size %zu\n", dest.size());
        return 1;
    }
    if (!expectPixel(dest, 4, 0, 0, src[0]) || !expectPixel(dest, 4, 1, 0, src[0]) ||
        !expectPixel(dest, 4, 2, 0, src[1]) || !expectPixel(dest, 4, 3, 1, src[1]) ||
        !expectPixel(dest, 4, 0, 2, src[2]) || !expectPixel(dest, 4, 3, 3, src[3])) {
        std::fprintf(stderr, "nearest-neighbor scale mismatch\n");
        return 2;
    }

    std::string error;
    const auto path = (std::filesystem::temp_directory_path() / "luma-preview-scale-check.png").string();
    if (!luma::encodePngArgb(dest.data(), 4, 4, path.c_str(), error)) {
        std::fprintf(stderr, "%s\n", error.c_str());
        return 3;
    }

    unsigned char signature[8]{};
    {
        std::ifstream in(path, std::ios::binary);
        in.read(reinterpret_cast<char*>(signature), 8);
        if (!in) {
            std::fprintf(stderr, "failed to read encoded PNG\n");
            return 4;
        }
    }
    const unsigned char png[8] = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};
    if (std::memcmp(signature, png, 8) != 0) {
        std::fprintf(stderr, "PNG signature mismatch\n");
        return 5;
    }
    std::error_code ec;
    std::filesystem::remove(path, ec);
    return 0;
}
