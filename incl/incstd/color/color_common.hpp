#pragma once

#ifdef _MSC_VER
#define NOMINMAX
#endif

#include <array>
#include <cstdint>
#include <string>
#include <sstream>
#include <iomanip>

namespace incom::standard::color {

struct inc_sRGB {
    std::uint8_t   r, g, b;
    constexpr bool operator==(const inc_sRGB &) const = default;
    constexpr inc_sRGB() : r(0), g(0), b(0) {}
    constexpr inc_sRGB(std::uint8_t const r, std::uint8_t const g, std::uint8_t const b) : r(r), g(g), b(b) {}
    constexpr inc_sRGB(std::array<std::uint8_t, 3> const other) : r(other[0]), g(other[1]), b(other[2]) {}

    constexpr std::string to_hex() const {
        std::ostringstream ss;
        ss << '#' << std::hex << std::setw(2) << std::setfill('0') << int(r) << std::setw(2) << std::setfill('0')
           << int(g) << std::setw(2) << std::setfill('0') << int(b);
        return ss.str();
    }
};

inline constexpr inc_sRGB const default_inc_sRGB{255, 255, 255};

struct inc_lRGB {
    double         r, g, b;
    constexpr bool operator==(const inc_lRGB &) const = default;

    constexpr inc_lRGB() : r(0.0), g(0.0), b(0.0) {}
    constexpr inc_lRGB(double const r, double const g, double const b) : r(r), g(g), b(b) {}
    constexpr inc_lRGB(std::array<double, 3> const other) : r(other[0]), g(other[1]), b(other[2]) {}
};

inline constexpr inc_lRGB const default_inc_lRGB{1.0, 1.0, 1.0};


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
