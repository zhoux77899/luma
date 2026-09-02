#include "preview-clipboard.h"

#include "preview-screenshot.h"

#include <SDL.h>
#include <SDL_syswm.h>

#include <X11/Xatom.h>
#include <X11/Xlib.h>

#include <vector>

namespace luma {
namespace {

std::vector<unsigned char> g_png;
Atom g_clipboard = None;
Atom g_targets = None;
Atom g_png_atom = None;
Atom g_image_png = None;

bool x11Info(SDL_Window* window, Display*& display, Window& xwindow) {
    if (window == nullptr) {
        return false;
    }
    SDL_SysWMinfo info;
    SDL_VERSION(&info.version);
    if (SDL_GetWindowWMInfo(window, &info) == SDL_FALSE) {
        return false;
    }
#ifdef SDL_VIDEO_DRIVER_X11
    if (info.subsystem != SDL_SYSWM_X11 || info.info.x11.display == nullptr) {
        return false;
    }
    display = info.info.x11.display;
    xwindow = info.info.x11.window;
    return true;
#else
    (void)display;
    (void)xwindow;
    return false;
#endif
}

void internAtoms(Display* display) {
    g_clipboard = XInternAtom(display, "CLIPBOARD", False);
    g_targets = XInternAtom(display, "TARGETS", False);
    g_png_atom = XInternAtom(display, "PNG", False);
    g_image_png = XInternAtom(display, "image/png", False);
}

void handleSelectionRequest(const XSelectionRequestEvent& request) {
    XSelectionEvent notify{};
    notify.type = SelectionNotify;
    notify.display = request.display;
    notify.requestor = request.requestor;
    notify.selection = request.selection;
    notify.target = request.target;
    notify.property = request.property == None ? request.target : request.property;
    notify.time = request.time;

    if (request.target == g_targets) {
        const Atom targets[] = {g_targets, g_png_atom, g_image_png};
        XChangeProperty(request.display, request.requestor, notify.property, XA_ATOM, 32, PropModeReplace,
                        reinterpret_cast<const unsigned char*>(targets), 3);
    } else if ((request.target == g_png_atom || request.target == g_image_png) && !g_png.empty()) {
        XChangeProperty(request.display, request.requestor, notify.property, request.target, 8, PropModeReplace,
                        g_png.data(), static_cast<int>(g_png.size()));
    } else {
        notify.property = None;
    }

    XSendEvent(request.display, request.requestor, True, NoEventMask, reinterpret_cast<XEvent*>(&notify));
    XFlush(request.display);
}

}  // namespace

bool copyPreviewScreenshot(SDL_Window* window, const uint32_t* argb, int width, int height, std::string& error) {
    Display* display = nullptr;
    Window xwindow = 0;
    if (!x11Info(window, display, xwindow)) {
        error = "Preview screenshot clipboard needs X11";
        return false;
    }
    if (!encodePngArgbToMemory(argb, width, height, g_png, error)) {
        return false;
    }

    internAtoms(display);
    XSetSelectionOwner(display, g_clipboard, xwindow, CurrentTime);
    if (XGetSelectionOwner(display, g_clipboard) != xwindow) {
        error = "Failed to own the clipboard";
        return false;
    }
    SDL_EventState(SDL_SYSWMEVENT, SDL_ENABLE);
    return true;
}

void pumpPreviewClipboard(SDL_Window* window) {
    if (window == nullptr) {
        return;
    }

    SDL_Event event;
    while (SDL_PeepEvents(&event, 1, SDL_GETEVENT, SDL_SYSWMEVENT, SDL_SYSWMEVENT) == 1) {
#ifdef SDL_VIDEO_DRIVER_X11
        if (event.syswm.msg == nullptr) {
            continue;
        }
        const XEvent& xevent = event.syswm.msg->msg.x11.event;
        if (xevent.type == SelectionRequest) {
            handleSelectionRequest(xevent.xselectionrequest);
            continue;
        }
        if (xevent.type == SelectionClear && xevent.xselectionclear.selection == g_clipboard) {
            g_png.clear();
        }
#endif
    }
}

}  // namespace luma
