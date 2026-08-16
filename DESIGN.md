# Luma Design Rules

This document is the single source of truth for Luma's visual design. All Logo, icon, interface-state, and promotional assets must use the color tokens defined here. Do not introduce ad hoc HEX values in implementations.

## 1. Color Sources

This palette is sourced from [Nippon Colors](https://nipponcolors.com/), a collection of traditional Japanese colors. The Japanese names, romanizations, and HEX mappings are cross-referenced through [COLORSEA's Nippon Colors name table](https://colorsea.js.org/zh/Names.html).

All values use six-digit uppercase HEX notation and assume sRGB. Unless stated otherwise, colors are fully opaque at `100%`.

## 2. Logo Invariants

- The Logo consists of four congruent petal shapes. Reuse one canonical geometry by rotating it around the center.
- The four petal centers are equally spaced, and all petals have equal area. Do not scale, stretch, or independently modify any petal's corner radius.
- The standard Logo uses this fixed position and color mapping: `Fuji` on top, `Momo` on the left, `Tamago` on the right, and `Byakuroku` on the bottom.
- Logo petals use only the Brand Core Colors below. Support colors, neutrals, and unregistered colors are outside the standard Logo palette.
- Use flat, opaque fills with no gradients, shadows, strokes, borders, text, or pixelation.
- Reserve black and dark colors for backgrounds, text, and other system roles. Keep them out of Logo petals, decorative shapes, and illustration subjects.
- White-background and dark-background Logo variants change only the canvas background. The four petals keep the same geometry, positions, and colors.

## 3. Brand Core Colors

These four colors are the official brand colors for the current Logo. Their values are immutable unless this document is intentionally revised as a brand decision.

| Token | Japanese name | Romanization | HEX | Logo position |
| --- | --- | --- | --- | --- |
| `brand-fuji` | 藤 | FUJI | `#8B81C3` | Top petal |
| `brand-momo` | 桃 | MOMO | `#F596AA` | Left petal |
| `brand-tamago` | 玉子 | TAMAGO | `#F9BF45` | Right petal |
| `brand-byakuroku` | 白緑 | BYAKUROKU | `#A8D8B9` | Bottom petal |

## 4. Full-Spectrum Support Colors

Support colors are for interaction states, categories, data visualization, and simple illustrations. They do not replace the four standard Logo colors. The set spans the spectrum from red through magenta, with a preference for bright, clear flat color blocks.

| Hue range | Token | Japanese name | Romanization | HEX | Recommended use |
| --- | --- | --- | --- | --- | --- |
| Red | `spectrum-benihi` | 紅緋 | BENIHI | `#F75C2F` | Emphasis, errors, heat |
| Orange | `spectrum-araisyu` | 洗朱 | ARAISYU | `#FB966E` | Warnings, warm categories |
| Yellow | `spectrum-yamabuki` | 山吹 | YAMABUKI | `#FFB11B` | Highlights, prompts, energy |
| Yellow-green | `spectrum-nae` | 苗 | NAE | `#86C166` | Growth, progress, positive states |
| Green | `spectrum-wakatake` | 若竹 | WAKATAKE | `#5DAC81` | Success, connection, stable states |
| Blue-green | `spectrum-aomidori` | 青緑 | AOMIDORI | `#00AA90` | Actions, confirmation, secondary emphasis |
| Cyan | `spectrum-mizu` | 水 | MIZU | `#81C7D4` | Information, calm, background accents |
| Blue | `spectrum-tsuyukusa` | 露草 | TSUYUKUSA | `#2EA9DF` | Links, information, interaction states |
| Indigo | `spectrum-benimidori` | 紅碧 | BENIMIDORI | `#7B90D2` | Navigation, secondary brand hierarchy |
| Violet | `brand-fuji` | 藤 | FUJI | `#8B81C3` | Brand identity, primary recognition |
| Magenta | `spectrum-tsutsuji` | 躑躅 | TSUTSUJI | `#E03C8A` | Active, creative, special emphasis |
| Pink | `brand-momo` | 桃 | MOMO | `#F596AA` | Soft emphasis, brand support |

### Support Color Rules

- Give each simple shape one color. Keep multiple fills out of a single shape.
- Keep adjacent color blocks close in area so that one color does not carry disproportionate visual weight.
- Keep interaction semantics consistent across the project: do not redefine error, warning, success, or information colors from one screen to another.
- Create additional hierarchy by choosing another registered color, not by adding a gradient.
- Check contrast before using a support color for text. On large light-colored surfaces, prefer `neutral-sumi` for body text; on dark surfaces, use `neutral-gofun`.

## 5. Neutral Colors

Neutrals are for canvases, typography, separators, system interfaces, and Logo presentation backgrounds. They are not part of the standard four-petal Logo palette.

| Token | Japanese name | Romanization | HEX | Role |
| --- | --- | --- | --- | --- |
| `neutral-gofun` | 胡粉 | GOFUN | `#FFFFFB` | Cool white, Light App canvas |
| `neutral-shironezumi` | 白鼠 | SHIRONEZUMI | `#BDC0BA` | Light card surface |
| `neutral-ginnezumi` | 銀鼠 | GINNEZUMI | `#91989F` | Dark and Light secondary text, separators, and disabled states |
| `neutral-sumi` | 墨 | SUMI | `#1C1C1C` | Light body text; Dark card surface |
| `neutral-kuro` | 黒 | KURO | `#080808` | Dark App canvas, Boot, and Logo presentation background |

### Neutral Color Rules

- Light Theme uses `neutral-gofun` as the App canvas and `neutral-shironezumi` as the card surface.
- Dark Theme uses `neutral-kuro` as the App canvas and `neutral-sumi` as the card surface. Define Settings pane cards by this fill, not by a stroke.
- Use `neutral-ginnezumi` for secondary text, separators, and disabled information in both Dark and Light. Do not use it for small body text.
- Use dark backgrounds only for an explicit dark mode or Logo presentation variant. A dark canvas is not part of the Logo mark itself.
- Do not introduce pure black `#000000` as a new design token. Use Nippon Colors' `neutral-kuro` when a black role is required.

## 6. Asset Handoff Checklist

Before submitting a new Logo, icon, or color-related asset, verify all of the following:

1. Every HEX value appears in this document and matches its registered value exactly.
2. The standard Logo still consists of four congruent, equally spaced, equal-area petals.
3. The asset contains no gradients, shadows, strokes, borders, text, or pixelated details.
4. Logo petals use no black, dark, neutral, support, or unregistered colors.
5. When a new color is needed, update this document before updating the asset.

The standard SVG assets are located in [`assets/luma-logo/`](assets/luma-logo/).
