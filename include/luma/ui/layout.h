#pragma once

#include "luma/core/display.h"

namespace luma {
namespace layout {

constexpr int kWidth = 240;
constexpr int kHeight = 135;
constexpr int kHeaderHeight = 24;
constexpr int kFooterHeight = 15;
constexpr int kHeaderLogoSize = 20;

constexpr Rect kHeader{0, 0, 240, kHeaderHeight};
constexpr Rect kFooter{0, kHeight - kFooterHeight, 240, kFooterHeight};
constexpr Rect kContentBoth{0, kHeaderHeight, 240, 96};
constexpr Rect kContentHeaderOnly{0, kHeaderHeight, 240, 111};
constexpr Rect kContentFooterOnly{0, 0, 240, 120};
constexpr Rect kContentFull{0, 0, 240, kHeight};

constexpr int kCardWidth = 111;
constexpr int kCardHeight = 22;
constexpr int kCardX0 = 5;
constexpr int kCardX1 = 124;
constexpr int kCardGapY = 4;

inline Rect appCardBounds(int column, int row) {
    const int x = column == 0 ? kCardX0 : kCardX1;
    const int y = kContentHeaderOnly.y + row * (kCardHeight + kCardGapY);
    return {x, y, kCardWidth, kCardHeight};
}

}  // namespace layout
}  // namespace luma
