#pragma once

#include "luma/core/display.h"

namespace luma {
namespace layout {

constexpr int kWidth = 240;
constexpr int kHeight = 135;
constexpr int kHeaderHeight = 32;
constexpr int kFooterHeight = 15;
constexpr int kHeaderLogoSize = 28;
constexpr int kHeaderLogoX = 6;
constexpr int kHeaderLogoY = 2;
constexpr int kChromeInset = 6;

constexpr Rect kHeader{0, 0, 240, kHeaderHeight};
constexpr Rect kFooter{0, kHeight - kFooterHeight, 240, kFooterHeight};
constexpr Rect kContentBoth{0, kHeaderHeight, 240, 88};
constexpr Rect kContentHeaderOnly{0, kHeaderHeight, 240, 103};
constexpr Rect kContentFooterOnly{0, 0, 240, 120};
constexpr Rect kContentFull{0, 0, 240, kHeight};

constexpr int kCardWidth = 111;
constexpr int kCardHeight = 22;
constexpr int kCardGapX = 6;
constexpr int kCardGapY = 3;
constexpr int kCardRadius = 4;
constexpr int kCardIconSize = 16;
constexpr int kCardIconRadius = 3;
constexpr int kCardRows = 4;
constexpr int kCardX0 = kCardGapX;
constexpr int kCardX1 = kCardGapX + kCardWidth + kCardGapX;

inline Rect appCardBounds(int column, int row) {
    const int x = column == 0 ? kCardX0 : kCardX1;
    const int y = kContentHeaderOnly.y + kCardGapY + row * (kCardHeight + kCardGapY);
    return {x, y, kCardWidth, kCardHeight};
}

}  // namespace layout
}  // namespace luma
