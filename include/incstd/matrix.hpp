#pragma once

#include <algorithm>

#include <more_concepts/more_concepts.hpp>

namespace incom::standard::matrix {
using namespace incom::standard;

/*Matrix rotation of 'indexed' random access containers.
Uses 'swapping in circles' method ... should be pretty fast
*/
template <typename T>
requires more_concepts::random_access_container<T> && more_concepts::random_access_container<typename T::value_type> &&
         std::swappable<typename T::value_type::value_type>
void matrixRotateLeft(T &VofVlike) {
    int sideLength = VofVlike.size() - 1;
    if (sideLength < 1) { return; }
    if (std::ranges::any_of(VofVlike, [&](auto const &line) { return line.size() != VofVlike.size(); })) { return; }

    int circles = (sideLength + 2) / 2;
    for (int cir = 0; cir < circles; cir++) {
        for (int i = 0; i < sideLength - (2 * cir); ++i) {
            std::swap(VofVlike[cir][cir + i], VofVlike[cir + i][sideLength - cir]);
            std::swap(VofVlike[cir + i][sideLength - cir], VofVlike[sideLength - cir][sideLength - cir - i]);
            std::swap(VofVlike[sideLength - cir][sideLength - cir - i], VofVlike[sideLength - cir - i][cir]);
        }
    }
    return;
}

template <typename T>
requires more_concepts::random_access_container<T> && more_concepts::random_access_container<typename T::value_type> &&
         std::swappable<typename T::value_type::value_type>
void matrixRotateRight(T &VofVlike) {
    int sideLength = VofVlike.size() - 1;
    if (sideLength < 1) { return; }
    if (std::ranges::any_of(VofVlike, [&](auto const &line) { return line.size() != VofVlike.size(); })) { return; }

    int circles = (sideLength + 2) / 2;
    for (int cir = 0; cir < circles; cir++) {
        for (int i = 0; i < sideLength - (2 * cir); ++i) {
            std::swap(VofVlike[cir][cir + i], VofVlike[sideLength - cir - i][cir]);
            std::swap(VofVlike[sideLength - cir - i][cir], VofVlike[sideLength - cir][sideLength - cir - i]);
            std::swap(VofVlike[sideLength - cir][sideLength - cir - i], VofVlike[cir + i][sideLength - cir]);
        }
    }
    return;
}

} // namespace incom::standard::matrix