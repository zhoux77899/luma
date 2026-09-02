#include "preview-actions.h"

#include "preview-clipboard.h"
#include "preview-report.h"
#include "preview-save-dialog.h"
#include "preview-screenshot.h"
#include "sdl-display-adapter.h"

#include <vector>

namespace luma {

void runPreviewCommand(PreviewCommand command, SdlDisplayAdapter& display) {
    if (command == PreviewCommand::None) {
        return;
    }

    std::vector<uint32_t> pixels;
    int width = 0;
    int height = 0;
    display.copyPresentedArgb(pixels, width, height);
    if (pixels.empty() || width <= 0 || height <= 0) {
        reportPreviewError(display.window(), "Preview canvas is not ready");
        return;
    }

    std::string error;
    if (command == PreviewCommand::CopyScreenshot) {
        if (!copyPreviewScreenshot(display.window(), pixels.data(), width, height, error)) {
            reportPreviewError(display.window(), error.c_str());
        }
        return;
    }

    const std::string directory = previewScreenshotDefaultDirectory();
    if (!ensureDirectory(directory, error)) {
        reportPreviewError(display.window(), error.c_str());
        return;
    }

    std::string path;
    const SaveDialogResult result =
        showSavePngDialog(display.window(), directory, previewScreenshotDefaultFilename(), path, error);
    if (result == SaveDialogResult::Cancelled) {
        return;
    }
    if (result == SaveDialogResult::Failed) {
        reportPreviewError(display.window(), error.c_str());
        return;
    }
    if (!encodePngArgb(pixels.data(), width, height, path.c_str(), error)) {
        reportPreviewError(display.window(), error.c_str());
    }
}

}  // namespace luma
