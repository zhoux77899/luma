#!/usr/bin/env python3
"""Encode the 10x10 Header battery outline and interior fill rows."""

from __future__ import annotations

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "src" / "luma" / "assets" / "battery-icons.cpp"

OUTLINE = [
    "..........",
    "....##....",
    "...####...",
    "..#....#..",
    "..#....#..",
    "..#....#..",
    "..#....#..",
    "..#....#..",
    "...####...",
    "..........",
]

# Interior well is rows 3-7, cols 3-6, filled from the bottom.
FILL_ROWS = [
    [3, 4, 5, 6],  # unused index 0
    [7],
    [6],
    [5],
    [4],
    [3],
]


def rows_to_bits(rows: list[str]) -> list[int]:
    out = []
    for row in rows:
        bits = 0
        for x, ch in enumerate(row):
            if ch == "#":
                bits |= 0x8000 >> x
        out.append(bits)
    return out


def fill_layer(row_index: int) -> list[str]:
    rows = ["." * 10 for _ in range(10)]
    chars = list(rows[row_index])
    for x in range(3, 7):
        chars[x] = "#"
    rows[row_index] = "".join(chars)
    return rows


def fmt(name: str, rows: list[int]) -> str:
    hexes = ", ".join(f"0x{row:04X}" for row in rows)
    return f"const uint16_t {name}[kBatteryIconSize] = {{\n    {hexes},\n}};\n"


def main() -> None:
    body = (
        '#include "luma/assets/battery-icons.h"\n\n'
        "namespace luma {\nnamespace assets {\n\n"
        + fmt("kBatteryOutline", rows_to_bits(OUTLINE))
        + "\n"
        + fmt("kBatteryFill1", rows_to_bits(fill_layer(7)))
        + "\n"
        + fmt("kBatteryFill2", rows_to_bits(fill_layer(6)))
        + "\n"
        + fmt("kBatteryFill3", rows_to_bits(fill_layer(5)))
        + "\n"
        + fmt("kBatteryFill4", rows_to_bits(fill_layer(4)))
        + "\n"
        + fmt("kBatteryFill5", rows_to_bits(fill_layer(3)))
        + "\n}  // namespace assets\n}  // namespace luma\n"
    )
    OUT.write_text(body, encoding="utf-8")
    print(f"wrote {OUT}")


if __name__ == "__main__":
    main()
