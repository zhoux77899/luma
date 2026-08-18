#!/usr/bin/env python3
"""Encode hand-tuned Wi-Fi and lock ASCII masks into C++ row bitmaps."""

from __future__ import annotations

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "src" / "luma" / "assets" / "wifi-icons.cpp"

# Header 16x16: origin bottom-left, arcs toward top-right.
DOT_16 = [
    "................",
    "................",
    "................",
    "................",
    "................",
    "................",
    "................",
    "................",
    "................",
    "................",
    "................",
    "................",
    "................",
    ".##.............",
    ".##.............",
    "................",
]

ARC1_16 = [
    "................",
    "................",
    "................",
    "................",
    "................",
    "................",
    "................",
    "................",
    "................",
    ".###............",
    ".#####..........",
    "....##..........",
    ".....##.........",
    ".....##.........",
    ".....##.........",
    "................",
]

ARC2_16 = [
    "................",
    "................",
    "................",
    "................",
    "................",
    ".####...........",
    ".######.........",
    ".....####.......",
    ".......##.......",
    "........##......",
    "........##......",
    ".........##.....",
    ".........##.....",
    ".........##.....",
    ".........##.....",
    "................",
]

ARC3_16 = [
    "................",
    ".######.........",
    ".########.......",
    ".......####.....",
    ".........##.....",
    "..........##....",
    "...........##...",
    "...........##...",
    "............##..",
    "............##..",
    ".............##.",
    ".............##.",
    ".............##.",
    ".............##.",
    ".............##.",
    "................",
]

# List 10x10 scheme B: 2x2 origin + mid + outer.
DOT_10 = [
    "..........",
    "..........",
    "..........",
    "..........",
    "..........",
    "..........",
    "..........",
    ".##.......",
    ".##.......",
    "..........",
]

ARC_MID_10 = [
    "..........",
    "..........",
    "..........",
    "..........",
    ".###......",
    "....#.....",
    ".....#....",
    ".....#....",
    ".....#....",
    "..........",
]

ARC_OUTER_10 = [
    "..........",
    ".####.....",
    ".....##...",
    ".......#..",
    ".......#..",
    "........#.",
    "........#.",
    "........#.",
    "........#.",
    "..........",
]

LOCK_CLOSED = [
    "..........",
    "....##....",
    "...#..#...",
    "...#..#...",
    "..######..",
    ".#......#.",
    ".#......#.",
    ".#......#.",
    "..######..",
    "..........",
]

LOCK_OPEN = [
    "..........",
    "....##....",
    "...#..#...",
    "...#......",
    "..######..",
    ".#......#.",
    ".#......#.",
    ".#......#.",
    "..######..",
    "..........",
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


def union_rows(*layers: list[str]) -> list[str]:
    n = len(layers[0])
    w = len(layers[0][0])
    merged = []
    for y in range(n):
        row = ""
        for x in range(w):
            row += "#" if any(layer[y][x] == "#" for layer in layers) else "."
        merged.append(row)
    return merged


def wifi_off(strong: list[str]) -> list[str]:
    n = len(strong)
    pts = {(x, y) for y, row in enumerate(strong) for x, ch in enumerate(row) if ch == "#"}
    slash = set()
    for i in range(n):
        slash.add((i, i))
        if i + 1 < n:
            slash.add((i + 1, i))
    gap = set()
    for x, y in slash:
        for dx in range(-2, 3):
            for dy in range(-2, 3):
                if abs(dx) + abs(dy) <= 2:
                    gap.add((x + dx, y + dy))
    off = (pts - gap) | slash
    rows = []
    for y in range(n):
        row = ""
        for x in range(n):
            row += "#" if (x, y) in off else "."
        rows.append(row)
    return rows


def fmt(name: str, rows: list[int]) -> str:
    hexes = ", ".join(f"0x{row:04X}" for row in rows)
    if len(rows) == 16:
        a = ", ".join(f"0x{row:04X}" for row in rows[:8])
        b = ", ".join(f"0x{row:04X}" for row in rows[8:])
        return f"const uint16_t {name}[kWifiIconSize] = {{\n    {a},\n    {b},\n}};\n"
    return f"const uint16_t {name}[kWifiListIconSize] = {{\n    {hexes},\n}};\n"


def main() -> None:
    off = wifi_off(union_rows(DOT_16, ARC1_16, ARC2_16, ARC3_16))
    body = (
        '#include "luma/assets/wifi-icons.h"\n\n'
        "namespace luma {\nnamespace assets {\n\n"
        + fmt("kWifiDisconnected", rows_to_bits(off))
        + "\n"
        + fmt("kWifiArc3", rows_to_bits(ARC3_16))
        + "\n"
        + fmt("kWifiArc2", rows_to_bits(ARC2_16))
        + "\n"
        + fmt("kWifiArc1", rows_to_bits(ARC1_16))
        + "\n"
        + fmt("kWifiDot", rows_to_bits(DOT_16))
        + "\n"
        + fmt("kWifiListDot", rows_to_bits(DOT_10))
        + "\n"
        + fmt("kWifiListArc2", rows_to_bits(ARC_MID_10))
        + "\n"
        + fmt("kWifiListArc3", rows_to_bits(ARC_OUTER_10))
        + "\n"
        + fmt("kLockClosed", rows_to_bits(LOCK_CLOSED))
        + "\n"
        + fmt("kLockOpen", rows_to_bits(LOCK_OPEN))
        + "\n}  // namespace assets\n}  // namespace luma\n"
    )
    OUT.write_text(body, encoding="utf-8")
    print(f"wrote {OUT}")


if __name__ == "__main__":
    main()
