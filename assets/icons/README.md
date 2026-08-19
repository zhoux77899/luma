# Header and list Wi-Fi icons

10 x 10 one-bit masks shared by the Header status cluster and Saved /
Scan SSID rows. Connected uses a 2x2 origin plus two arcs (scheme B);
Weakest paints every layer with Theme secondary text. Every
non-connected Header state uses the same Strong mark with a diagonal
slash. Lock / lock-open mark credential presence on Saved and Scan
rows. Regenerate with `python tools/gen-wifi-icons.py`. The committed
bitmaps live in `src/luma/assets/wifi-icons.cpp` and are painted at
runtime with Theme `primary_text` / `secondary_text`.
