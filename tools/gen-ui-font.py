#!/usr/bin/env python3
"""Extract ASCII and zh_hans CJK from Fusion Pixel 10px BDFs.

Writes include/luma/ui/font.h, include/luma/ui/font-cjk.h, and
src/luma/ui/font-cjk.cpp. The firmware build does not run this script.
"""

from __future__ import annotations

import argparse
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
ASSETS = ROOT / "assets" / "fonts"
INCLUDE = ROOT / "include" / "luma" / "ui" / "font.h"
CJK_HEADER = ROOT / "include" / "luma" / "ui" / "font-cjk.h"
CJK_SOURCE = ROOT / "src" / "luma" / "ui" / "font-cjk.cpp"

FIRST = 0x20
LAST = 0x7F
COUNT = LAST - FIRST + 1
FONT_HEIGHT = 10
FONT_BOTTOM_Y = -1
CJK_WIDTH = 10


def parse_ascii_glyphs(path: Path) -> tuple[int, int, list[list[int]]]:
    text = path.read_text(encoding="utf-8")
    glyphs: dict[int, list[int]] = {}
    encoding: int | None = None
    width = 0
    height = 0
    in_bitmap = False
    rows: list[int] = []

    def commit() -> None:
        nonlocal encoding, in_bitmap, rows
        if encoding is not None and FIRST <= encoding <= LAST:
            glyphs[encoding] = rows
        encoding = None
        in_bitmap = False
        rows = []

    for raw in text.splitlines():
        line = raw.strip()
        if line.startswith("ENCODING "):
            encoding = int(line.split()[1])
        elif line.startswith("DWIDTH ") and encoding is not None and FIRST <= encoding <= LAST:
            width = max(width, int(line.split()[1]))
        elif line.startswith("BBX ") and encoding is not None and FIRST <= encoding <= LAST:
            parts = line.split()
            height = max(height, int(parts[2]))
        elif line == "BITMAP":
            in_bitmap = encoding is not None and FIRST <= encoding <= LAST
            rows = []
        elif line == "ENDCHAR":
            commit()
        elif in_bitmap:
            bits = int(line, 16)
            shift = 16 - len(line) * 4
            if shift < 0:
                bits >>= -shift
                shift = 0
            rows.append(bits << shift)

    if width <= 0 or height <= 0:
        raise SystemExit(f"{path}: could not determine glyph size")
    if 0x3F not in glyphs:
        raise SystemExit(f"{path}: missing '?' glyph")

    table = []
    for code in range(FIRST, LAST + 1):
        if code in glyphs:
            table.append(glyphs[code])
            continue
        if code == 0x7F:
            table.append([0] * height)
            continue
        raise SystemExit(f"{path}: missing ASCII glyph {code}")
    for rows in table:
        if len(rows) != height:
            raise SystemExit(f"{path}: expected {height} rows, got {len(rows)}")
    return width, height, table


def write_ascii_bdf(path: Path, family: str, version: str, width: int, height: int,
                    table: list[list[int]]) -> None:
    lines = [
        "STARTFONT 2.1",
        f"FONT -Luma-{family}-10-100-75-75-C-{width * 10}-ISO10646-1",
        "SIZE 10 75 75",
        f"FONTBOUNDINGBOX {width} {height} 0 -1",
        "STARTPROPERTIES 3",
        f"FAMILY_NAME \"{family}\"",
        f"FONT_VERSION \"{version}\"",
        "COPYRIGHT \"Derived from Fusion Pixel Font / Fusion Bold Pixel Font (OFL-1.1)\"",
        "ENDPROPERTIES",
        f"CHARS {COUNT}",
    ]
    for index, rows in enumerate(table):
        code = FIRST + index
        lines.extend(
            [
                f"STARTCHAR u{code:04X}",
                f"ENCODING {code}",
                f"DWIDTH {width} 0",
                f"BBX {width} {height} 0 -1",
                "BITMAP",
            ]
        )
        for row in rows:
            hex_width = (width + 3) // 4
            bits = row >> (16 - hex_width * 4)
            lines.append(f"{bits:0{hex_width}X}")
        lines.append("ENDCHAR")
    lines.append("ENDFONT")
    lines.append("")
    path.write_text("\n".join(lines), encoding="utf-8")


def format_table(name: str, table: list[list[int]]) -> str:
    blocks = []
    for index, rows in enumerate(table):
        code = FIRST + index
        glyph = ", ".join(f"0x{row:04X}" for row in rows)
        char = chr(code)
        if char == " ":
            label = "space"
        elif char == "\\":
            label = "backslash"
        elif code == 0x7F:
            label = "DEL"
        else:
            label = char
        blocks.append(f"    {{{glyph}}},  // {label}")
    body = "\n".join(blocks)
    return f"static const uint16_t {name}[{COUNT}][kGlyphHeight] = {{\n{body}\n}};\n"


def include_cjk_code(code: int) -> bool:
    return (0x2000 <= code <= 0x206F or 0x3000 <= code <= 0x303F or
            0x4E00 <= code <= 0x9FFF or 0xFF00 <= code <= 0xFFEF)


def rasterize_bdf_bitmap(hex_rows: list[str], bbx_w: int, bbx_h: int, bbx_x: int,
                         bbx_y: int) -> list[int]:
    top_y = FONT_BOTTOM_Y + FONT_HEIGHT - 1
    out = [0] * FONT_HEIGHT
    for index, raw in enumerate(hex_rows):
        y = bbx_y + bbx_h - 1 - index
        row_index = top_y - y
        if row_index < 0 or row_index >= FONT_HEIGHT:
            continue
        bits = int(raw, 16)
        bitlen = len(raw) * 4
        row = 0
        for column in range(bbx_w):
            mask = 1 << (bitlen - 1 - column)
            if (bits & mask) == 0:
                continue
            out_col = bbx_x + column
            if 0 <= out_col < 16:
                row |= 0x8000 >> out_col
        out[row_index] = row
    return out


def parse_cjk_glyphs(path: Path) -> list[tuple[int, list[int]]]:
    glyphs: dict[int, list[int]] = {}
    encoding: int | None = None
    bbx_w = 0
    bbx_h = 0
    bbx_x = 0
    bbx_y = 0
    in_bitmap = False
    hex_rows: list[str] = []

    def commit() -> None:
        nonlocal encoding, in_bitmap, hex_rows
        if encoding is not None and include_cjk_code(encoding) and hex_rows:
            glyphs[encoding] = rasterize_bdf_bitmap(hex_rows, bbx_w, bbx_h, bbx_x, bbx_y)
        encoding = None
        in_bitmap = False
        hex_rows = []

    for raw in path.read_text(encoding="utf-8", errors="replace").splitlines():
        line = raw.strip()
        if line.startswith("ENCODING "):
            encoding = int(line.split()[1])
        elif line.startswith("BBX ") and encoding is not None and include_cjk_code(encoding):
            parts = line.split()
            bbx_w = int(parts[1])
            bbx_h = int(parts[2])
            bbx_x = int(parts[3])
            bbx_y = int(parts[4])
        elif line == "BITMAP":
            in_bitmap = encoding is not None and include_cjk_code(encoding)
            hex_rows = []
        elif line == "ENDCHAR":
            commit()
        elif in_bitmap:
            hex_rows.append(line)

    if not glyphs:
        raise SystemExit(f"{path}: no zh_hans CJK glyphs found")
    return sorted(glyphs.items())


def write_cjk_header() -> None:
    CJK_HEADER.parent.mkdir(parents=True, exist_ok=True)
    CJK_HEADER.write_text(
        """#pragma once

#include <cstdint>

namespace luma {
namespace font {

constexpr int kCjkGlyphWidth = 10;

const uint16_t* cjkGlyphRows(uint32_t code);
int cjkGlyphCount();

}  // namespace font
}  // namespace luma
""",
        encoding="utf-8",
    )


def write_cjk_source(glyphs: list[tuple[int, list[int]]]) -> None:
    CJK_SOURCE.parent.mkdir(parents=True, exist_ok=True)
    lines = [
        '#include "luma/ui/font-cjk.h"',
        "",
        "#include <cstddef>",
        "",
        "namespace luma {",
        "namespace font {",
        "",
        "namespace {",
        "",
        "struct CjkGlyph {",
        "    uint16_t code;",
        "    uint16_t rows[10];",
        "};",
        "",
        f"const CjkGlyph kCjk[{len(glyphs)}] = {{",
    ]
    for code, rows in glyphs:
        packed = ", ".join(f"0x{row:04X}" for row in rows)
        lines.append(f"    {{0x{code:04X}, {{{packed}}}}},")
    lines.extend(
        [
            "};",
            "",
            "}  // namespace",
            "",
            "const uint16_t* cjkGlyphRows(uint32_t code) {",
            "    if (code > 0xFFFFu) {",
            "        return nullptr;",
            "    }",
            "    const uint16_t key = static_cast<uint16_t>(code);",
            "    std::size_t lo = 0;",
            f"    std::size_t hi = {len(glyphs)};",
            "    while (lo < hi) {",
            "        const std::size_t mid = lo + (hi - lo) / 2;",
            "        if (kCjk[mid].code < key) {",
            "            lo = mid + 1;",
            "        } else {",
            "            hi = mid;",
            "        }",
            "    }",
            f"    if (lo < {len(glyphs)} && kCjk[lo].code == key) {{",
            "        return kCjk[lo].rows;",
            "    }",
            "    return nullptr;",
            "}",
            "",
            f"int cjkGlyphCount() {{ return {len(glyphs)}; }}",
            "",
            "}  // namespace font",
            "}  // namespace luma",
            "",
        ]
    )
    CJK_SOURCE.write_text("\n".join(lines), encoding="utf-8")


def write_header(regular_w: int, bold_w: int, height: int, regular: list[list[int]],
                 bold: list[list[int]]) -> None:
    header = f"""#pragma once

#include "luma/core/display.h"
#include "luma/ui/font-cjk.h"

#include <cstdint>

// Luma UI bitmap font: ASCII 0x20-0x7F from Fusion Pixel 10px monospaced latin
// (regular) and Fusion Bold Pixel 10px monospaced latin (bold), plus the
// Fusion Pixel 10px monospaced zh_hans common set. OFL-1.1.
// Regenerated by tools/gen-ui-font.py. The firmware build does not run that script.
namespace luma {{
namespace font {{

constexpr int kGlyphWidth = {regular_w};
constexpr int kBoldGlyphWidth = {bold_w};
constexpr int kGlyphHeight = {height};
constexpr int kFirstCode = {FIRST};
constexpr int kGlyphCount = {COUNT};

struct Glyph {{
    const uint16_t* rows;
    int width;
}};

{format_table("kRegular", regular)}
{format_table("kBold", bold)}
inline const uint16_t* glyphRows(char character, bool bold) {{
    unsigned char code = static_cast<unsigned char>(character);
    if (code < kFirstCode || code > {LAST}) {{
        code = static_cast<unsigned char>('?');
    }}
    return (bold ? kBold : kRegular)[code - kFirstCode];
}}

inline int glyphWidth(bool bold) {{
    return bold ? kBoldGlyphWidth : kGlyphWidth;
}}

inline bool nextCodepoint(const char*& cursor, uint32_t& code) {{
    if (cursor == nullptr || *cursor == '\\0') {{
        return false;
    }}
    const unsigned char lead = static_cast<unsigned char>(*cursor);
    auto trail = [](unsigned char byte) {{ return (byte & 0xC0u) == 0x80u; }};
    if (lead < 0x80u) {{
        code = lead;
        ++cursor;
        return true;
    }}
    if ((lead & 0xE0u) == 0xC0u) {{
        const unsigned char b1 = static_cast<unsigned char>(cursor[1]);
        if (b1 == 0 || !trail(b1)) {{
            code = 0xFFFDu;
            ++cursor;
            return true;
        }}
        code = (static_cast<uint32_t>(lead & 0x1Fu) << 6) | (b1 & 0x3Fu);
        cursor += 2;
        return true;
    }}
    if ((lead & 0xF0u) == 0xE0u) {{
        const unsigned char b1 = static_cast<unsigned char>(cursor[1]);
        const unsigned char b2 = b1 == 0 ? 0 : static_cast<unsigned char>(cursor[2]);
        if (b1 == 0 || b2 == 0 || !trail(b1) || !trail(b2)) {{
            code = 0xFFFDu;
            ++cursor;
            return true;
        }}
        code = (static_cast<uint32_t>(lead & 0x0Fu) << 12) |
               (static_cast<uint32_t>(b1 & 0x3Fu) << 6) | (b2 & 0x3Fu);
        cursor += 3;
        return true;
    }}
    if ((lead & 0xF8u) == 0xF0u) {{
        const unsigned char b1 = static_cast<unsigned char>(cursor[1]);
        const unsigned char b2 = b1 == 0 ? 0 : static_cast<unsigned char>(cursor[2]);
        const unsigned char b3 = b2 == 0 ? 0 : static_cast<unsigned char>(cursor[3]);
        if (b1 == 0 || b2 == 0 || b3 == 0 || !trail(b1) || !trail(b2) || !trail(b3)) {{
            code = 0xFFFDu;
            ++cursor;
            return true;
        }}
        code = (static_cast<uint32_t>(lead & 0x07u) << 18) |
               (static_cast<uint32_t>(b1 & 0x3Fu) << 12) |
               (static_cast<uint32_t>(b2 & 0x3Fu) << 6) | (b3 & 0x3Fu);
        cursor += 4;
        return true;
    }}
    code = 0xFFFDu;
    ++cursor;
    return true;
}}

inline Glyph glyphFor(uint32_t code, bool bold) {{
    if (code >= static_cast<uint32_t>(kFirstCode) && code <= {LAST}u) {{
        const unsigned char ascii = static_cast<unsigned char>(code);
        return {{(bold ? kBold : kRegular)[ascii - kFirstCode], glyphWidth(bold)}};
    }}
    const uint16_t* rows = cjkGlyphRows(code);
    if (rows != nullptr) {{
        return {{rows, kCjkGlyphWidth}};
    }}
    return {{glyphRows('?', bold), glyphWidth(bold)}};
}}

inline int textWidth(const char* text, int size, bool bold = false) {{
    if (text == nullptr) {{
        return 0;
    }}
    const int scale = size < 1 ? 1 : size;
    int width = 0;
    const char* cursor = text;
    uint32_t code = 0;
    while (nextCodepoint(cursor, code)) {{
        width += glyphFor(code, bold).width;
    }}
    return width * scale;
}}

template <typename SetPixel>
void drawText(Point origin, TextStyle style, const char* text, SetPixel set_pixel) {{
    if (text == nullptr) {{
        return;
    }}
    const int scale = style.size < 1 ? 1 : style.size;
    int x = origin.x;
    const char* cursor = text;
    uint32_t code = 0;
    while (nextCodepoint(cursor, code)) {{
        const Glyph glyph = glyphFor(code, style.bold);
        for (int row = 0; row < kGlyphHeight; ++row) {{
            for (int column = 0; column < glyph.width; ++column) {{
                if ((glyph.rows[row] & static_cast<uint16_t>(0x8000u >> column)) == 0) {{
                    continue;
                }}
                for (int dy = 0; dy < scale; ++dy) {{
                    for (int dx = 0; dx < scale; ++dx) {{
                        set_pixel(x + column * scale + dx, origin.y + row * scale + dy,
                                  style.color);
                    }}
                }}
            }}
        }}
        x += glyph.width * scale;
    }}
}}

}}  // namespace font
}}  // namespace luma
"""
    INCLUDE.parent.mkdir(parents=True, exist_ok=True)
    INCLUDE.write_text(header, encoding="utf-8")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--regular-bdf", type=Path, help="Upstream regular latin BDF")
    parser.add_argument("--bold-bdf", type=Path, help="Upstream bold latin BDF")
    parser.add_argument("--zh-hans-bdf", type=Path, help="Upstream Fusion Pixel zh_hans BDF")
    args = parser.parse_args()

    ASSETS.mkdir(parents=True, exist_ok=True)
    regular_src = args.regular_bdf or ASSETS / "luma-ui-10px-regular.bdf"
    bold_src = args.bold_bdf or ASSETS / "luma-ui-10px-bold.bdf"
    zh_src = args.zh_hans_bdf
    if zh_src is None:
        candidate = ASSETS / "fusion-pixel-10px-monospaced-zh_hans.bdf"
        zh_src = candidate if candidate.exists() else None

    regular_w, regular_h, regular = parse_ascii_glyphs(regular_src)
    bold_w, bold_h, bold = parse_ascii_glyphs(bold_src)
    if regular_h != bold_h:
        raise SystemExit(f"regular and bold glyph heights differ: {regular_h} vs {bold_h}")

    write_ascii_bdf(ASSETS / "luma-ui-10px-regular.bdf", "Luma UI", "regular", regular_w,
                    regular_h, regular)
    write_ascii_bdf(ASSETS / "luma-ui-10px-bold.bdf", "Luma UI Bold", "bold", bold_w, bold_h,
                    bold)
    write_header(regular_w, bold_w, regular_h, regular, bold)
    print(f"wrote {INCLUDE} (regular {regular_w}x{regular_h}, bold {bold_w}x{bold_h}, {COUNT} glyphs)")

    if zh_src is None:
        if not CJK_SOURCE.exists():
            raise SystemExit("zh_hans BDF required to generate src/luma/ui/font-cjk.cpp")
        print(f"kept existing {CJK_SOURCE}")
        return

    glyphs = parse_cjk_glyphs(zh_src)
    write_cjk_header()
    write_cjk_source(glyphs)
    print(f"wrote {CJK_SOURCE} ({len(glyphs)} zh_hans glyphs)")


if __name__ == "__main__":
    main()
