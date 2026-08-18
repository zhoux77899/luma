# Header and list Wi-Fi icons

16 x 16 one-bit masks used by the Header status cluster. Connected
uses three 45-degree arc layers plus a 2x2 origin; Weakest lights only
the origin. Every non-connected state uses the same Strong mark with a
diagonal slash. Saved and Scan SSID rows use 10 x 10 masks: a 2x2
origin plus two arcs (scheme B), and lock / lock-open for credential
presence. Weakest paints every 10 x 10 layer with Theme secondary
text. Regenerate with `python tools/gen-wifi-icons.py`. The committed
bitmaps live in `src/luma/assets/wifi-icons.cpp` and are painted at
runtime with Theme `primary_text` / `secondary_text`.
