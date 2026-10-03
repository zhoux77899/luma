#!/usr/bin/env python3
"""Rasterize redistributable Wikimedia Commons wordmarks for REMOTE."""

from __future__ import annotations

import json
import urllib.parse
import urllib.request
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SVG_DIR = ROOT / "assets" / "brand-marks"
HEADER = ROOT / "include" / "luma" / "assets" / "brand-marks.h"
SOURCE = ROOT / "src" / "luma" / "assets" / "brand-marks.cpp"

# Commons file titles whose pages carry a public-domain text-logo tag.
FILES = [
    ("lg", "LG logo (2023).svg"),
    ("tcl", "Logo of the TCL Corporation.svg"),
    ("hisense", "Hisense logo.svg"),
    ("samsung", "Samsung logo wordmark.svg"),
    ("sony", "Sony logo.svg"),
    ("panasonic", "Panasonic logo.svg"),
    ("philips", "Philips logo.svg"),
]

MAX_W = 96
MAX_H = 28
FREE_MARKERS = ("pd-textlogo", "pd-logo", "pd-ineligible", "cc0", "public domain")


def commons_meta(title: str) -> dict:
    query = urllib.parse.urlencode(
        {
            "action": "query",
            "titles": f"File:{title}",
            "prop": "imageinfo",
            "iiprop": "url|extmetadata",
            "format": "json",
        }
    )
    url = "https://commons.wikimedia.org/w/api.php?" + query
    request = urllib.request.Request(url, headers={"User-Agent": "luma-brand-marks/1.0 (firmware asset generator)"})
    with urllib.request.urlopen(request, timeout=30) as response:
        payload = json.load(response)
    pages = payload["query"]["pages"]
    page = next(iter(pages.values()))
    if "missing" in page:
        raise FileNotFoundError(title)
    info = page["imageinfo"][0]
    meta = info.get("extmetadata", {})
    license_text = " ".join(
        str(item.get("value", "")) for item in meta.values() if isinstance(item, dict)
    ).lower()
    return {"url": info["url"], "license": license_text, "descriptionurl": info.get("descriptionurl", "")}


def download(url: str, dest: Path) -> None:
    request = urllib.request.Request(url, headers={"User-Agent": "luma-brand-marks/1.0"})
    with urllib.request.urlopen(request, timeout=60) as response:
        dest.write_bytes(response.read())


def rasterize(svg_path: Path) -> tuple[int, int, bytes]:
    from io import BytesIO

    import cairosvg
    from PIL import Image

    png = cairosvg.svg2png(url=str(svg_path), output_width=MAX_W * 4)
    image = Image.open(BytesIO(png)).convert("RGBA")
    scale = min(MAX_W / image.width, MAX_H / image.height, 1)
    width = max(1, int(image.width * scale))
    height = max(1, int(image.height * scale))
    image = image.resize((width, height), Image.Resampling.LANCZOS)
    return width, height, image.tobytes()


def emit(marks: list[tuple[str, int, int, bytes] | None]) -> None:
    lines = [
        '#include "luma/assets/brand-marks.h"',
        "",
        "// Rasterized from redistributable Wikimedia Commons SVGs in assets/brand-marks/.",
        "// Near-black and near-white pixels are remapped to the Theme preference at draw time.",
        "",
        "namespace luma {",
        "namespace assets {",
        "namespace {",
        "",
    ]
    slots = []
    for index, mark in enumerate(marks):
        if mark is None:
            slots.append(None)
            continue
        slug, width, height, pixels = mark
        symbol = f"kMark{index}"
        slots.append((symbol, width, height))
        lines.append(f"constexpr uint8_t {symbol}[{width * height * 4}] = {{")
        chunk = []
        for value in pixels:
            chunk.append(str(value))
            if len(chunk) == 16:
                lines.append("    " + ", ".join(chunk) + ",")
                chunk = []
        if chunk:
            lines.append("    " + ", ".join(chunk) + ",")
        lines.append("};")
        lines.append("")
    lines.append("}  // namespace")
    lines.append("")
    lines.append("const RgbaMark* brandMark(int index) {")
    lines.append("    static const RgbaMark kMarks[] = {")
    for slot in slots:
        if slot is None:
            lines.append("        {0, 0, nullptr},")
        else:
            symbol, width, height = slot
            lines.append(f"        {{{width}, {height}, {symbol}}},")
    lines.append("    };")
    lines.append("    if (index < 0 || index >= static_cast<int>(sizeof(kMarks) / sizeof(kMarks[0]))) {")
    lines.append("        return nullptr;")
    lines.append("    }")
    lines.append("    if (kMarks[index].rgba == nullptr) {")
    lines.append("        return nullptr;")
    lines.append("    }")
    lines.append("    return &kMarks[index];")
    lines.append("}")
    lines.append("")
    lines.append("}  // namespace assets")
    lines.append("}  // namespace luma")
    lines.append("")
    SOURCE.write_text("\n".join(lines), encoding="utf-8")


def main() -> None:
    SVG_DIR.mkdir(parents=True, exist_ok=True)
    marks: list[tuple[str, int, int, bytes] | None] = []
    notes = []
    found = 0
    for slug, title in FILES:
        svg_path = SVG_DIR / f"{slug}.svg"
        if svg_path.exists() and svg_path.stat().st_size > 100:
            width, height, pixels = rasterize(svg_path)
            marks.append((slug, width, height, pixels))
            found += 1
            notes.append(f"{slug}: {width}x{height} local {svg_path.name}")
            continue
        try:
            meta = commons_meta(title)
        except Exception as error:  # noqa: BLE001 - record the miss and keep the name fallback
            marks.append(None)
            notes.append(f"{slug}: missing ({error})")
            continue
        if not any(marker in meta["license"] for marker in FREE_MARKERS):
            marks.append(None)
            notes.append(f"{slug}: skipped, license is not redistributable ({meta['descriptionurl']})")
            continue
        svg_path = SVG_DIR / f"{slug}.svg"
        download(meta["url"], svg_path)
        width, height, pixels = rasterize(svg_path)
        marks.append((slug, width, height, pixels))
        found += 1
        notes.append(f"{slug}: {width}x{height} {meta['descriptionurl']}")
    if found == 0:
        raise SystemExit("no redistributable marks\n" + "\n".join(notes))
    emit(marks)
    print("\n".join(notes))


if __name__ == "__main__":
    main()
