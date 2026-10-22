#pragma once

#include <algorithm>
#include <expected>
#include <ranges>

#include <more_concepts/more_concepts.hpp>

namespace incom::standard::matrix {
using namespace incom::standard;

/*Matrix rotation of 'indexed' random access containers.
Uses 'swapping in circles' method ... should be pretty fast
*/
template <typename T>
requires more_concepts::random_access_container<T> && more_concepts::random_access_container<typename T::value_type> &&
         std::swappable<typename T::value_type::value_type>
void
matrixRotateLeft(T &VofVlike) {
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
void
matrixRotateRight(T &VofVlike) {
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

template <typename T>
requires more_concepts::random_access_container<T> && more_concepts::random_access_container<typename T::value_type> &&
         std::is_nothrow_default_constructible_v<typename T::value_type>
auto
matrixRotateLeft_copy(T const &VofVlike) -> std::expected<std::remove_cvref_t<T>, int> {
    if (VofVlike.size() == 0uz || VofVlike.front().size() == 0 ||
        std::ranges::any_of(std::views::pairwise(VofVlike),
                            [](auto const &a) { return std::get<0>(a).size() != std::get<1>(a).size(); })) {
        return std::unexpected{1};
    }

    T res(VofVlike.front().size());
    for (auto &resLine : res) { resLine.reserve(VofVlike.size()); }

    for (size_t yRes{}; yRes < VofVlike.front().size(); ++yRes) {
        for (size_t xRes{}; xRes < VofVlike.size(); ++xRes) {
            res[yRes].push_back(VofVlike[xRes][VofVlike.front().size() - yRes - 1]);
        }
    }
    return res;
}

template <typename T>
requires more_concepts::random_access_container<T> && more_concepts::random_access_container<typename T::value_type> &&
         std::is_nothrow_default_constructible_v<typename T::value_type>
auto
matrixRotateRight_copy(T const &VofVlike) -> std::expected<std::remove_cvref_t<T>, int> {
    if (VofVlike.size() == 0uz || VofVlike.front().size() == 0 ||
        std::ranges::any_of(std::views::pairwise(VofVlike),
                            [](auto const &a) { return std::get<0>(a).size() != std::get<1>(a).size(); })) {
        return std::unexpected{1};
    }

    T res(VofVlike.front().size());
    for (auto &resLine : res) { resLine.reserve(VofVlike.size()); }

    for (size_t yRes{}; yRes < VofVlike.front().size(); ++yRes) {
        for (size_t xRes{}; xRes < VofVlike.size(); ++xRes) {
            res[yRes].push_back(VofVlike[VofVlike.size() - xRes - 1][yRes]);
        }
    }
    return res;
}

} // namespace incom::standard::matrix