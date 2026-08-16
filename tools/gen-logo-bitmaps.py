#!/usr/bin/env python3
"""Rasterize assets/luma-logo/luma-logo.svg into committed RGB565 headers."""

from __future__ import annotations

import math
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
INCLUDE = ROOT / "include" / "luma" / "assets"

PETAL = [
    (500.0, 100.0),
    (509.0, 100.0),
    (518.0, 107.0),
    (528.0, 116.0),
    (570.0, 153.0),
    (612.0, 201.0),
    (636.0, 247.0),
    (657.0, 294.0),
    (648.0, 342.0),
    (610.0, 382.0),
    (577.0, 416.0),
    (544.0, 451.0),
    (510.0, 484.0),
    (504.0, 490.0),
    (496.0, 490.0),
    (490.0, 484.0),
    (456.0, 451.0),
    (423.0, 416.0),
    (390.0, 382.0),
    (352.0, 342.0),
    (343.0, 294.0),
    (364.0, 247.0),
    (388.0, 201.0),
    (430.0, 153.0),
    (472.0, 116.0),
    (482.0, 107.0),
    (491.0, 100.0),
    (500.0, 100.0),
]

PETAL_SCALE = 0.95
PETAL_SCALE_ANCHOR = (500.0, 100.0)

COLORS = (
    ((0x8B, 0x81, 0xC3), 0.0),
    ((0xF9, 0xBF, 0x45), 90.0),
    ((0xA8, 0xD8, 0xB9), 180.0),
    ((0xF5, 0x96, 0xAA), 270.0),
)


def cubic(p0, p1, p2, p3, steps: int):
    points = []
    for i in range(steps + 1):
        t = i / steps
        u = 1.0 - t
        x = (u**3) * p0[0] + 3 * (u**2) * t * p1[0] + 3 * u * (t**2) * p2[0] + (t**3) * p3[0]
        y = (u**3) * p0[1] + 3 * (u**2) * t * p1[1] + 3 * u * (t**2) * p2[1] + (t**3) * p3[1]
        points.append((x, y))
    return points


def flatten_petal(steps: int = 24):
    points = []
    cursor = PETAL[0]
    index = 1
    while index + 2 < len(PETAL):
        points.extend(cubic(cursor, PETAL[index], PETAL[index + 1], PETAL[index + 2], steps)[:-1])
        cursor = PETAL[index + 2]
        index += 3
    points.append(cursor)
    return points


def rotate(point, degrees: float):
    angle = math.radians(degrees)
    x, y = point[0] - 500.0, point[1] - 500.0
    return (
        500.0 + x * math.cos(angle) - y * math.sin(angle),
        500.0 + x * math.sin(angle) + y * math.cos(angle),
    )


def scale_from_outer_tip(point):
    anchor_x, anchor_y = PETAL_SCALE_ANCHOR
    return (
        anchor_x + (point[0] - anchor_x) * PETAL_SCALE,
        anchor_y + (point[1] - anchor_y) * PETAL_SCALE,
    )


def contains(polygon, x: float, y: float) -> bool:
    inside = False
    j = len(polygon) - 1
    for i, (xi, yi) in enumerate(polygon):
        xj, yj = polygon[j]
        intersects = ((yi > y) != (yj > y)) and (x < (xj - xi) * (y - yi) / (yj - yi + 1e-12) + xi)
        if intersects:
            inside = not inside
        j = i
    return inside


def to_rgb565(r: int, g: int, b: int) -> int:
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)


def rasterize(size: int) -> list[int]:
    base_petal = [scale_from_outer_tip(point) for point in flatten_petal()]
    petals = [(color, [rotate(point, degrees) for point in base_petal]) for color, degrees in COLORS]
    scale = 1000.0 / size
    pixels = []
    for y in range(size):
        for x in range(size):
            sample_x = (x + 0.5) * scale
            sample_y = (y + 0.5) * scale
            value = 0
            for color, polygon in petals:
                if contains(polygon, sample_x, sample_y):
                    value = to_rgb565(*color)
                    break
            pixels.append(value)
    return pixels


def write_header(path: Path, symbol: str, size: int, pixels: list[int]) -> None:
    lines = [
        "#pragma once",
        "",
        "#include <cstdint>",
        "",
        "namespace luma {",
        "namespace assets {",
        "",
        f"constexpr int k{symbol}Size = {size};",
        f"constexpr uint16_t k{symbol}[{size * size}] = {{",
    ]
    row = []
    for pixel in pixels:
        row.append(f"0x{pixel:04X}")
        if len(row) == 16:
            lines.append("    " + ", ".join(row) + ",")
            row = []
    if row:
        lines.append("    " + ", ".join(row) + ",")
    lines.extend(["};", "", "}  // namespace assets", "}  // namespace luma", ""])
    path.write_text("\n".join(lines), encoding="utf-8")


def main() -> None:
    INCLUDE.mkdir(parents=True, exist_ok=True)
    write_header(INCLUDE / "luma-logo-boot.h", "LogoBoot", 96, rasterize(96))
    write_header(INCLUDE / "luma-logo-header.h", "LogoHeader", 28, rasterize(28))


if __name__ == "__main__":
    main()
