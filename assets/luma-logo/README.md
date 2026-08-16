# Luma Logo SVG Assets

The logo uses one shared rounded-petal path and four rotations around the exact center `(500, 500)`. Each petal is uniformly scaled to `95%` around its outer tip `(500, 100)` before rotation. This preserves the original soft rounded-square proportions while moving the inner corners away from the center for clear rendering at both 96 px and 28 px.

Core colors:

- Fuji: `#8B81C3`
- Momo: `#F596AA`
- Tamago: `#F9BF45`
- Byakuroku: `#A8D8B9`

Files:

- `luma-logo.svg`: transparent background
- `luma-logo-white.svg`: white background
- `luma-logo-black.svg`: black background

Firmware uses committed RGB565 headers generated from the same canonical geometry as `luma-logo.svg`:

```powershell
python tools/gen-logo-bitmaps.py
```

That writes `include/luma/assets/luma-logo-boot.h` (96 x 96) and
`include/luma/assets/luma-logo-header.h` (28 x 28). The firmware build does not
run this script.
