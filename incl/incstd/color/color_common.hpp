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


} // namespace incom::standard::color
