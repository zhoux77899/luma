# Shared UTF-8 text uses Fusion Pixel zh_hans, not M5GFX CJK

SSID and other UI strings are UTF-8. Drawing them through Cardputer's CJK face would match neither the SDL preview nor ADR-0007. Embedding Fusion Pixel 10px monospaced zh_hans on the same blit keeps hosts pixel-identical: latin stays 5px, CJK is 10px, and a missing codepoint is one `?`. A GB2312 dump or a device-only font would either bloat the table or break the shared seam.
