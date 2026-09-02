#include "preview-clipboard.h"

#include "preview-screenshot.h"

#import <Cocoa/Cocoa.h>

#include <vector>

namespace luma {

bool copyPreviewScreenshot(SDL_Window*, const uint32_t* argb, int width, int height, std::string& error) {
    std::vector<unsigned char> png;
    if (!encodePngArgbToMemory(argb, width, height, png, error)) {
        return false;
    }

    @autoreleasepool {
        NSData* data = [NSData dataWithBytes:png.data() length:png.size()];
        NSPasteboard* pasteboard = [NSPasteboard generalPasteboard];
        [pasteboard clearContents];
        if (![pasteboard setData:data forType:NSPasteboardTypePNG]) {
            error = "Failed to copy Preview screenshot";
            return false;
        }
    }
    return true;
}

void pumpPreviewClipboard(SDL_Window*) {}

}  // namespace luma
