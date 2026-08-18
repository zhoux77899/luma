# Header Wi-Fi icons

16 x 16 one-bit masks used by the Header status cluster. Connected
uses three Lucide-style arc layers plus a separate center dot; Weakest
lights only the dot. Every non-connected state uses the Lucide wifi-off
mask. Regenerate with `python tools/gen-wifi-icons.py`. The committed
bitmaps live in `src/luma/assets/wifi-icons.cpp` and are painted at
runtime with Theme `primary_text` / `secondary_text`.
