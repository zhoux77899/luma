#include "preview-menu.h"

#import <Cocoa/Cocoa.h>

#include <SDL.h>

namespace luma {
namespace {

PreviewCommand g_menu_command = PreviewCommand::None;

}  // namespace
}  // namespace luma

@interface LumaPreviewMenuTarget : NSObject
- (void)copyScreenshot:(id)sender;
- (void)saveScreenshot:(id)sender;
@end

@implementation LumaPreviewMenuTarget
- (void)copyScreenshot:(id)sender {
    (void)sender;
    luma::g_menu_command = luma::PreviewCommand::CopyScreenshot;
}
- (void)saveScreenshot:(id)sender {
    (void)sender;
    luma::g_menu_command = luma::PreviewCommand::SaveScreenshot;
}
@end

namespace luma {
namespace {

LumaPreviewMenuTarget* menuTarget() {
    static LumaPreviewMenuTarget* target = [[LumaPreviewMenuTarget alloc] init];
    return target;
}

NSMenuItem* makeItem(NSString* title, SEL selector, NSString* key, NSEventModifierFlags flags) {
    NSMenuItem* item = [[NSMenuItem alloc] initWithTitle:title action:selector keyEquivalent:key];
    [item setKeyEquivalentModifierMask:flags];
    [item setTarget:menuTarget()];
    return item;
}

}  // namespace

void attachPreviewMenu(SDL_Window*) {
    @autoreleasepool {
        NSMenu* main_menu = [[NSMenu alloc] initWithTitle:@"MainMenu"];
        NSMenuItem* app_item = [[NSMenuItem alloc] init];
        NSMenu* app_menu = [[NSMenu alloc] initWithTitle:@"Luma SDL preview"];
        [app_item setSubmenu:app_menu];
        [main_menu addItem:app_item];

        NSMenuItem* file_item = [[NSMenuItem alloc] init];
        NSMenu* file_menu = [[NSMenu alloc] initWithTitle:@"File"];
        [file_menu addItem:makeItem(@"Copy Screenshot", @selector(copyScreenshot:), @"c",
                                    NSEventModifierFlagCommand | NSEventModifierFlagShift)];
        [file_menu addItem:makeItem(@"Save Screenshot…", @selector(saveScreenshot:), @"s",
                                    NSEventModifierFlagCommand | NSEventModifierFlagShift)];
        [file_item setSubmenu:file_menu];
        [main_menu addItem:file_item];

        [NSApp setMainMenu:main_menu];
    }
}

int previewMenuTopInset() { return 0; }

bool handlePreviewMenuEvent(const SDL_Event&, PreviewCommand&) { return false; }

PreviewCommand takePreviewMenuCommand() {
    const PreviewCommand command = g_menu_command;
    g_menu_command = PreviewCommand::None;
    return command;
}

void drawPreviewMenuOverlay(SDL_Renderer*, int, int) {}

}  // namespace luma
