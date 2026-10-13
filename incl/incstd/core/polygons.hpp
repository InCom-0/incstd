#pragma once

#include <cstdlib>
#include <numeric>
#include <ranges>

#include <incstd/core/concepts.hpp>


namespace incom::standard::polygons {
using namespace incom::standard;

template <bool AllBorders = false, typename C>
requires std::ranges::forward_range<C> &&
         requires(std::ranges::range_reference_t<C> p, std::ranges::range_reference_t<C> q) {
             requires concepts::is_pair_like<decltype(p)>;
             { static_cast<__int128>(std::get<0>(p)) };
             { static_cast<__int128>(std::get<1>(p)) };
             { p != q } -> std::convertible_to<bool>;
         }
size_t
compute_areaFromVerts(C const &verts) {
    auto abs_i128 = [](__int128 val) -> __int128 { return val < 0 ? -val : val; };
    auto abs_u64  = [](long long val) -> unsigned long long {
        if (val >= 0ll) { return static_cast<unsigned long long>(val); }
        return static_cast<unsigned long long>(-(val + 1ll)) + 1ull;
    };

    __int128 twiceArea{};
    __int128 boundaryPoints{};

    auto firstIt = std::ranges::begin(verts);
    auto endIt   = std::ranges::end(verts);
    if (firstIt == endIt) { return 0uz; }

    auto prevIt = firstIt;
    auto it     = prevIt;
    ++it;
    if (it == endIt) { return 0uz; }

    size_t vertCount = 1uz;

    for (; it != endIt; ++it, ++prevIt) {
        auto const &curr = *prevIt;
        auto const &next = *it;

        twiceArea += static_cast<__int128>(std::get<0>(curr)) * static_cast<__int128>(std::get<1>(next)) -
                     static_cast<__int128>(std::get<1>(curr)) * static_cast<__int128>(std::get<0>(next));

        if constexpr (AllBorders) {
            auto const dY   = static_cast<long long>(std::get<0>(next) - std::get<0>(curr));
            auto const dX   = static_cast<long long>(std::get<1>(next) - std::get<1>(curr));
            boundaryPoints += static_cast<__int128>(std::gcd(abs_u64(dY), abs_u64(dX)));
        }

        ++vertCount;
    }

    if (vertCount < 3uz) { return 0uz; }

    if (*prevIt != *firstIt) {
        auto const &last  = *prevIt;
        auto const &first = *firstIt;

        twiceArea += static_cast<__int128>(std::get<0>(last)) * static_cast<__int128>(std::get<1>(first)) -
                     static_cast<__int128>(std::get<1>(last)) * static_cast<__int128>(std::get<0>(first));

        if constexpr (AllBorders) {
            auto const dY   = static_cast<long long>(std::get<0>(first) - std::get<0>(last));
            auto const dX   = static_cast<long long>(std::get<1>(first) - std::get<1>(last));
            boundaryPoints += static_cast<__int128>(std::gcd(abs_u64(dY), abs_u64(dX)));
        }
    }

    if constexpr (AllBorders) { return static_cast<size_t>((abs_i128(twiceArea) + boundaryPoints) / 2 + 1); }
    else { return static_cast<size_t>(abs_i128(twiceArea) / 2); }
}
} // namespace incom::standard::polygons