# Luma UI font

The firmware and SDL preview draw ASCII through a committed bitmap subset of:

- [Fusion Pixel Font](https://github.com/TakWolf/fusion-pixel-font) 10px monospaced latin (regular)
- [Fusion Bold Pixel Font](https://github.com/pixel-font-studio/fusion-bold-pixel-font) 10px monospaced latin (bold)

Both upstream fonts are SIL Open Font License 1.1. The embedded subset is named Luma UI so it does not use the upstream family names. License texts are in `OFL-fusion-pixel.txt` and `OFL-fusion-bold-pixel.txt`.

Vendored sources:

- `luma-ui-10px-regular.bdf`
- `luma-ui-10px-bold.bdf`

Regenerate the committed header after changing those BDFs:

```powershell
python tools/gen-ui-font.py
```

That writes `include/luma/ui/font.h`. The firmware build does not run this script. This milestone embeds ASCII `0x20-0x7F` only.
