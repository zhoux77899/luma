#!/usr/bin/env python3
"""Encode the locked 12x12 GitHub Octocat ASCII mask into a C++ row bitmap."""

from __future__ import annotations

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "src" / "luma" / "assets" / "github-icon.cpp"

# Locked About Repository mark: Octocat silhouette only, two empty rows on top.
GITHUB = [
    "............",
    "............",
    "...##..##...",
    "...######...",
    "..########..",
    "..########..",
    "..########..",
    "...######...",
    "..#.####....",
    "...#####....",
    "....####....",
    "....####....",
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


def main() -> None:
    hexes = ", ".join(f"0x{row:04X}" for row in rows_to_bits(GITHUB))
    body = (
        '#include "luma/assets/github-icon.h"\n\n'
        "namespace luma {\nnamespace assets {\n\n"
        "const uint16_t kGithubIcon[kGithubIconSize] = {\n"
        f"    {hexes},\n"
        "};\n\n"
        "}  // namespace assets\n}  // namespace luma\n"
    )
    OUT.write_text(body, encoding="utf-8")
    print(f"wrote {OUT}")


if __name__ == "__main__":
    main()
