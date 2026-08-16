# SDL preview reuses Core through an injected host factory

The `luma-sdl-preview` CMake target compiles `include/luma` and `src/luma` while excluding Cardputer adapters. Preview `main.cpp` constructs `Luma` with injected SDL and host adapters so the firmware-only default constructor in `luma-factory.cpp` is not linked twice. The preview depends on SDL2 only; text is drawn with an embedded bitmap font so host-only TTF packages stay out of the firmware set.
