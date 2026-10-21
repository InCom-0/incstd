#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>


// Shared direction / offset primitives for the solvers (explorers, pathfinders, ...)
namespace incom::standard::solvers::directions {

template <size_t Dims>
using Pos = std::array<size_t, Dims>;

// #######################################
// ### GENERIC DIRECTIONAL OFFSET TABLES
// #######################################

// Axis-aligned unit steps: 2 * Dims offsets in the order +dim0, -dim0, +dim1, -dim1, ...
template <size_t Dims, typename INT = long long, INT StepSz = 1LL>
requires(std::is_signed<INT>::value) && (Dims > 0uz)
inline consteval auto
get_dirChanges() {
    constexpr size_t posCount = Dims * 2uz;

    std::array<std::array<INT, Dims>, posCount> res{};
    for (size_t oneDir = 0uz; oneDir < Dims; ++oneDir) {
        res[oneDir * 2][oneDir]       = StepSz;
        res[(oneDir * 2) + 1][oneDir] = (-1 * StepSz);
    }
    return res;
}

template <typename INT = long long, INT StepSz = 1LL>
requires(std::is_signed<INT>::value)
inline consteval auto
get_dirChanges_2D() {
    return get_dirChanges<2uz, INT, StepSz>();
}

template <typename INT = long long, INT StepSZ = 1LL>
requires(std::is_signed<INT>::value)
inline consteval auto
get_dirChanges_3D() {
    return get_dirChanges<3uz, INT, StepSZ>();
}

// Moore neighbourhood: all 3^Dims - 1 offsets with every component in {-1, 0, 1} * StepSz, in lexicographic order
// of the base-3 digits (dim0 varies fastest). Not cyclically ordered, use a dedicated enum where that matters.
template <size_t Dims, typename INT = long long, INT StepSz = 1LL>
requires(std::is_signed<INT>::value) && (Dims > 0uz)
inline consteval auto
get_dirChanges_moore() {
    constexpr size_t total = []() {
        size_t n = 1uz;
        for (size_t i = 0uz; i < Dims; ++i) { n *= 3uz; }
        return n;
    }();

    std::array<std::array<INT, Dims>, total - 1uz> res{};
    size_t                                         out = 0uz;
    for (size_t code = 0uz; code < total; ++code) {
        std::array<INT, Dims> off{};
        size_t                rest   = code;
        bool                  nonzero = false;
        for (size_t d = 0uz; d < Dims; ++d) {
            auto const digit = static_cast<INT>(rest % 3uz);
            rest /= 3uz;
            off[d]  = (digit - 1) * StepSz;
            nonzero = nonzero || (digit != 1);
        }
        if (nonzero) { res[out++] = off; }
    }
    return res;
}


// ####################################
// ### 2D EIGHT-WAY DIRECTION
// ####################################

// Ordered cyclically so that neighbouring enumerators are neighbouring compass points, even values are orthogonal,
// odd values diagonal. Offsets are {row, col}.
enum class Dir : std::uint8_t {
    Down,
    DownRight,
    Right,
    UpRight,
    Up,
    UpLeft,
    Left,
    DownLeft
};

inline constexpr std::array<int, 8uz> c_dRow{1, 1, 0, -1, -1, -1, 0, 1};
inline constexpr std::array<int, 8uz> c_dCol{0, 1, 1, 1, 0, -1, -1, -1};

[[nodiscard]] constexpr size_t
ix(Dir d) noexcept {
    return static_cast<size_t>(d);
}

[[nodiscard]] constexpr int
row_step(Dir d) noexcept {
    return c_dRow[ix(d)];
}

[[nodiscard]] constexpr int
col_step(Dir d) noexcept {
    return c_dCol[ix(d)];
}

[[nodiscard]] constexpr bool
is_diagonal(Dir d) noexcept {
    return (ix(d) & 1uz) != 0uz;
}

[[nodiscard]] constexpr Dir
rotated(Dir d, int eighths) noexcept {
    return static_cast<Dir>((static_cast<int>(d) + eighths) & 7);
}

// Vertical / horizontal components of a diagonal direction
[[nodiscard]] constexpr Dir
vertical_of(Dir d) noexcept {
    return row_step(d) > 0 ? Dir::Down : Dir::Up;
}

[[nodiscard]] constexpr Dir
horizontal_of(Dir d) noexcept {
    return col_step(d) > 0 ? Dir::Right : Dir::Left;
}

// The table has to be exactly the 2D Moore neighbourhood
static_assert([]() {
    constexpr auto moore = get_dirChanges_moore<2uz, int>();
    for (size_t i = 0uz; i < 8uz; ++i) {
        bool found = false;
        for (auto const &m : moore) { found = found || (m[0] == c_dRow[i] && m[1] == c_dCol[i]); }
        if (!found) { return false; }
    }
    return true;
}());

} // namespace incom::standard::solvers::directions
