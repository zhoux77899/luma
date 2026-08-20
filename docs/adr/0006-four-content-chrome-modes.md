# Four Content chrome modes replace the RFC single band

RFC #1 fixed Header 18 / Content 101 / Footer 16. v0.1 uses Header 32 and Footer 15 so Content can be 88 (both), 103 (Header only), 120 (Footer only), or 135 (full). Boot is full, Launcher is Header only with a 4-row card grid, and Apps keep Header plus Footer. Left and right content inset is 6px to match Launcher cards. Footer key names sit in `spectrum-aomidori` chips; action labels follow. Dialog is a rounded card (`kCardRadius`) vertically centered in Content, not a full-canvas overlay.
