# Embed rasterized Logo bitmaps instead of parsing SVG

The 240 x 135 firmware canvas cannot parse SVG at runtime. Luma commits RGB565 bitmaps generated from `assets/luma-logo/luma-logo.svg` and draws them through `DisplaySurface::drawBitmap`, keeping the SVG as the source of truth and a regen script out of the firmware build.
