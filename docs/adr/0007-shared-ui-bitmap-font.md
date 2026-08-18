# UI text is a shared Fusion Pixel blit, not M5GFX Font0

Cardputer Font0 is 6 x 8 and the SDL preview used a different 8 x 8 table, so text could not match across hosts. Luma now blits a committed ASCII subset of Fusion Pixel 10px monospaced latin plus Fusion Bold through `DisplaySurface::drawText`, keeping preview and firmware pixel-identical and giving App card initials a true bold face. ADR-0011 extends that same blit with UTF-8 and the zh_hans CJK set. The firmware build does not run the generator.
