#pragma once

// =============================================================================
// DiceFont - 5x7 digit glyphs drawn into the 8x8 pad grid
// =============================================================================
// Seven-segment-ish digits for 1..6. Each glyph is 7 rows of 5 bits (bit 4 =
// leftmost column). drawDigit() centres it in the 8x8 grid; remember row 0 is
// the BOTTOM row on the Launchpad, so the font's top row maps to grid row 7.
// =============================================================================

#include "PadGrid.h"

#include <array>
#include <cstdint>

namespace dicefont {

// glyphs[n] is digit n (1..6); index 0 is unused padding.
inline const std::array<std::array<uint8_t, 7>, 7>& glyphs() {
    static const std::array<std::array<uint8_t, 7>, 7> g = {{
        {{0, 0, 0, 0, 0, 0, 0}},  // 0 - unused
        {{0b00100, 0b01100, 0b00100, 0b00100, 0b00100, 0b00100, 0b01110}},  // 1
        {{0b01110, 0b10001, 0b00001, 0b00010, 0b00100, 0b01000, 0b11111}},  // 2
        {{0b11111, 0b00010, 0b00100, 0b00010, 0b00001, 0b10001, 0b01110}},  // 3
        {{0b00010, 0b00110, 0b01010, 0b10010, 0b11111, 0b00010, 0b00010}},  // 4
        {{0b11111, 0b10000, 0b11110, 0b00001, 0b00001, 0b10001, 0b01110}},  // 5
        {{0b00110, 0b01000, 0b10000, 0b11110, 0b10001, 0b10001, 0b01110}},  // 6
    }};
    return g;
}

// Paint digit n (1..6) into the grid in the given palette colour.
inline void drawDigit(PadGrid& grid, int n, int color) {
    if (n < 1 || n > 6) return;
    const auto& gl = glyphs()[n];
    constexpr int colOffset = 2;  // 5-wide glyph, roughly centred in 8 columns
    for (int fr = 0; fr < 7; ++fr) {
        uint8_t bits = gl[fr];
        int gridRow = 7 - fr;     // font's top row -> grid's top row
        for (int c = 0; c < 5; ++c)
            if (bits & (1 << (4 - c)))
                grid.set(colOffset + c, gridRow, color);
    }
}

}  // namespace dicefont
