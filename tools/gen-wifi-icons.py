#!/usr/bin/env python3
"""Rasterize Lucide wifi / wifi-off into 16x16 one-bit masks that fill the canvas."""

from __future__ import annotations

import math
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "src" / "luma" / "assets" / "wifi-icons.cpp"

CENTER = (12.0, 20.0)
STROKE = 1.05
DOT_R = 1.35


def sample_arc(r: float, y_chord: float, n: int = 96) -> list[tuple[float, float]]:
    cx, cy = CENTER
    half = math.sqrt(max(0.0, r * r - (cy - y_chord) ** 2))
    a0 = math.atan2(y_chord - cy, -half)
    a1 = math.atan2(y_chord - cy, half)
    pts = []
    for i in range(n + 1):
        a = a0 + (a1 - a0) * i / n
        pts.append((cx + r * math.cos(a), cy + r * math.sin(a)))
    return pts


def tight_bbox(layers: list[str]) -> tuple[float, float, float, float]:
    pts: list[tuple[float, float]] = []
    if "outer" in layers:
        pts.extend(sample_arc(15, 8.82))
    if "mid" in layers:
        pts.extend(sample_arc(10, 12.859))
    if "inner" in layers:
        pts.extend(sample_arc(5, 16.429))
    if "dot" in layers:
        pts.append(CENTER)
    if "slash" in layers:
        pts.extend([(2.0, 2.0), (22.0, 22.0)])
    pad = 1.15
    xs = [p[0] for p in pts]
    ys = [p[1] for p in pts]
    return min(xs) - pad, min(ys) - pad, max(xs) + pad, max(ys) + pad


def on_arc(px: float, py: float, r: float, y_chord: float) -> bool:
    cx, cy = CENTER
    if abs(math.hypot(px - cx, py - cy) - r) > STROKE:
        return False
    if py > y_chord + 0.35 or py > cy:
        return False
    return True


def dist_seg(px: float, py: float, x1: float, y1: float, x2: float, y2: float) -> float:
    vx, vy = x2 - x1, y2 - y1
    wx, wy = px - x1, py - y1
    den = vx * vx + vy * vy
    t = 0.0 if den == 0 else max(0.0, min(1.0, (wx * vx + wy * vy) / den))
    return math.hypot(px - (x1 + t * vx), py - (y1 + t * vy))


def raster(kind: str, bbox: tuple[float, float, float, float]) -> list[int]:
    x0, y0, x1, y1 = bbox
    rows = []
    for y in range(16):
        bits = 0
        for x in range(16):
            px = x0 + (x + 0.5) / 16.0 * (x1 - x0)
            py = y0 + (y + 0.5) / 16.0 * (y1 - y0)
            hit = False
            if kind == "outer":
                hit = on_arc(px, py, 15, 8.82)
            elif kind == "mid":
                hit = on_arc(px, py, 10, 12.859)
            elif kind == "inner":
                hit = on_arc(px, py, 5, 16.429)
            elif kind == "dot":
                hit = math.hypot(px - CENTER[0], py - CENTER[1]) <= DOT_R
            elif kind == "off":
                slash = dist_seg(px, py, 2, 2, 22, 22) <= 1.15
                wifi = (
                    on_arc(px, py, 15, 8.82)
                    or on_arc(px, py, 10, 12.859)
                    or on_arc(px, py, 5, 16.429)
                    or math.hypot(px - CENTER[0], py - CENTER[1]) <= DOT_R
                )
                if slash:
                    hit = True
                elif wifi and dist_seg(px, py, 2, 2, 22, 22) >= 2.2:
                    hit = True
            if hit:
                bits |= 0x8000 >> x
        rows.append(bits)
    return rows


def fmt(name: str, rows: list[int]) -> str:
    hexes = ", ".join(f"0x{row:04X}" for row in rows[:8])
    hexes2 = ", ".join(f"0x{row:04X}" for row in rows[8:])
    return f"const uint16_t {name}[kWifiIconSize] = {{\n    {hexes},\n    {hexes2},\n}};\n"


def main() -> None:
    wifi_bbox = tight_bbox(["outer", "mid", "inner", "dot"])
    off_bbox = tight_bbox(["outer", "mid", "inner", "dot", "slash"])
    body = (
        '#include "luma/assets/wifi-icons.h"\n\n'
        "namespace luma {\nnamespace assets {\n\n"
        + fmt("kWifiDisconnected", raster("off", off_bbox))
        + "\n"
        + fmt("kWifiArc3", raster("outer", wifi_bbox))
        + "\n"
        + fmt("kWifiArc2", raster("mid", wifi_bbox))
        + "\n"
        + fmt("kWifiArc1", raster("inner", wifi_bbox))
        + "\n"
        + fmt("kWifiDot", raster("dot", wifi_bbox))
        + "\n}  // namespace assets\n}  // namespace luma\n"
    )
    OUT.write_text(body, encoding="utf-8")
    print(f"wrote {OUT}")


if __name__ == "__main__":
    main()
