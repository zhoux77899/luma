# Luma UI font

The firmware and SDL preview draw text through a committed bitmap subset of:

- [Fusion Pixel Font](https://github.com/TakWolf/fusion-pixel-font) 10px monospaced latin (regular)
- [Fusion Bold Pixel Font](https://github.com/pixel-font-studio/fusion-bold-pixel-font) 10px monospaced latin (bold)
- Fusion Pixel 10px monospaced `zh_hans` (Simplified Chinese common set)

Upstream fonts are SIL Open Font License 1.1. The embedded subset is named Luma UI so it does not use the upstream family names. License texts are in `OFL-fusion-pixel.txt` and `OFL-fusion-bold-pixel.txt`.

Vendored latin sources:

- `luma-ui-10px-regular.bdf`
- `luma-ui-10px-bold.bdf`

Latin cells stay 5px (regular) or 6px (bold) wide. CJK cells are 10px wide. `drawText` decodes UTF-8; a missing codepoint becomes one `?`.

Regenerate the committed tables after changing the BDFs. Download the upstream zh_hans BDF from the [Fusion Pixel releases](https://github.com/TakWolf/fusion-pixel-font/releases) (`fusion-pixel-font-10px-monospaced-bdf`) and pass it in:

```powershell
python tools/gen-ui-font.py --zh-hans-bdf path\to\fusion-pixel-10px-monospaced-zh_hans.bdf
```

That writes `include/luma/ui/font.h`, `include/luma/ui/font-cjk.h`, and `src/luma/ui/font-cjk.cpp`. The firmware build does not run this script. Do not commit the upstream zh_hans BDF.
