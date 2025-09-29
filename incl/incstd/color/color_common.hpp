#pragma once

#include <array>
#include <cstdint>

namespace incom::standard::color {

struct INCCC_RBG {
    std::uint8_t   r, g, b;
    constexpr bool operator==(const INCCC_RBG &) const = default;
};

using inc_sRGB = std::array<std::uint8_t, 3>;
using inc_lRGB = std::array<double, 3>;

using palette16  = std::array<inc_sRGB, 16>;
using palette256 = std::array<inc_sRGB, 256>;

enum class ANSI_Color16 {
    Black = 0,
    Red,
    Green,
    Yellow,
    Blue,
    Magenta,
    Cyan,
    White,
    Bright_Black,
    Bright_Red,
    Bright_Green,
    Bright_Yellow,
    Bright_Blue,
    Bright_Magenta,
    Bright_Cyan,
    Bright_White,
};

inline constexpr inc_sRGB get_fromPalette(palette16 const &palette, ANSI_Color16 toGet) {
    return palette[static_cast<int>(toGet)];
}
inline constexpr inc_sRGB get_fromPalette(palette256 const &palette, ANSI_Color16 toGet) {
    return palette[static_cast<int>(toGet)];
}


} // namespace incom::standard::color
