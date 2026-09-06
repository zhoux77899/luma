# Product

<!-- impeccable:product-schema 1 -->

## Platform

web

## Stack

Vite + React, Tailwind CSS v4, Motion, shadcn, and the public `@beui` registry. Static export to GitHub Pages. Chosen over Next.js for a lighter Pages deploy; chosen over VitePress because beUI is React-only.

## Users

People who own or are flashing a Cardputer ADV and need the User guide that matches the LUMA they run, including an unreleased main-branch draft. Contributors who check the same pages while editing the handbook.

## Product Purpose

LUMA is the firmware product that runs on Cardputer ADV. The User guide site publishes the User guide with a product cover so a reader can flash a release and use Launcher, Settings, Notes, and DOTS.

## Positioning

The handbook is one tree on main. The site rebuilds each GitHub Release from that tag and treats `latest` as current main, so the repo never stores per-release copies.

## Operating Context

Readers arrive from the GitHub README, a Release page, or About on device (Build identity `X.Y.Z` or `X.Y.Z.{commit}`). They pick a language, a version, and a topic. Authors edit `docs/user-manual/` in English and 简体中文. GitHub Pages hosts `https://zhoux77899.github.io/luma/`.

## Capabilities and Constraints

- Cover plus User guide only. No blog, changelog marketing, or download storefront beyond linking GitHub Releases.
- Every non-draft Release appears in the switcher. A tag without `docs/user-manual/` is a stub.
- `latest` is always main. `/main/` is not a public path.
- `/` follows `zh*` vs other browser languages and remembers the choice.
- Search, left directory, right on-this-page headings.
- beUI chrome on the cover and shell; guide bodies stay quiet Markdown.
- Only the public `@beui` registry. No beUI Pro.
- Colors and Logo follow DESIGN.md. ADR, DESIGN.md, and agent docs stay off the site.

## Brand Commitments

Product spelling is LUMA. Logo is four congruent petals (Fuji, Momo, Tamago, Byakuroku) with no gradients, shadows, strokes, or text on the mark. Palette is the Nippon Colors tokens in DESIGN.md.

## Evidence on Hand

- User guide source: `docs/user-manual/` (en, zh, shared PNGs)
- Logo SVG: `assets/luma-logo/`
- Firmware screenshots in `docs/user-manual/assets/`
- Releases v0.1.0 and v0.1.1 have no User guide in-tree
- Do not invent testimonials, download counts, or hardware claims beyond Cardputer ADV

## Product Principles

- The flashed Build identity should map to a versioned User guide, or to an honest stub.
- One handbook source; history lives in git tags, not in duplicated repo trees.
- Cover may persuade; reading pages must stay scannable.
- Brand tokens stay registered. No ad hoc HEX.

## Accessibility & Inclusion

Bilingual 简体中文 and English. Language is user-selectable after the first automatic choice. Honor `prefers-reduced-motion` on beUI motion.
