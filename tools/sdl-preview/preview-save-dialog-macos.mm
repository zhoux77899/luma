#include "preview-save-dialog.h"

#import <Cocoa/Cocoa.h>

namespace luma {

SaveDialogResult showSavePngDialog(SDL_Window*, const std::string& directory, const std::string& filename,
                                   std::string& path, std::string& error) {
    @autoreleasepool {
        NSSavePanel* panel = [NSSavePanel savePanel];
        [panel setAllowedFileTypes:@[ @"png" ]];
        [panel setAllowsOtherFileTypes:NO];
        [panel setCanCreateDirectories:YES];
        [panel setNameFieldStringValue:[NSString stringWithUTF8String:filename.c_str()]];
        if (!directory.empty()) {
            NSURL* directory_url = [NSURL fileURLWithPath:[NSString stringWithUTF8String:directory.c_str()]
                                              isDirectory:YES];
            [panel setDirectoryURL:directory_url];
        }

        const NSModalResponse response = [panel runModal];
        if (response != NSModalResponseOK) {
            return SaveDialogResult::Cancelled;
        }

        NSURL* url = [panel URL];
        if (url == nil || [url path] == nil) {
            error = "Failed to read the save path";
            return SaveDialogResult::Failed;
        }
        path = [[url path] UTF8String];
        return SaveDialogResult::Saved;
    }
}

}  // namespace luma
