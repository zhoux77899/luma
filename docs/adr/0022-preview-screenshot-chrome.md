# Preview screenshot chrome is native on Windows/macOS and a reserved strip on Linux

SDL2 has no cross-platform menu API. An in-window strip would have been one implementation, but the SDL preview uses a native Preview menu bar on Windows and macOS. Linux has no equivalent for an SDL2/X11 window, so the same File commands sit on a reserved-top Preview menu strip instead of pulling GTK or another toolkit into the preview. The preview still depends on SDL2 only; PNG encode stays in a vendored write helper, not SDL_image.
