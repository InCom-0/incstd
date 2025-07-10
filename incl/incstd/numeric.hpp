#pragma once

#include <cmath>
#include <cstdlib>
#include <ranges>
#include <type_traits>


namespace incom::standard::numeric {
using namespace incom::standard;

template <typename T>
requires std::is_integral_v<T>
int get_numOfDigits(T const &integralNumber) {
    if constexpr (std::is_unsigned_v<T>) { return integralNumber == 0 ? 1 : std::trunc(log10(integralNumber)) + 1; }
    else { return integralNumber == 0 ? 1 : std::trunc(log10(std::abs(integralNumber))) + 1; }
}

template <typename T>
requires std::is_arithmetic_v<std::ranges::range_value_t<T>>
auto compute_variance(T &range) {
    using v_t    = std::ranges::range_value_t<T>;
    v_t    sum   = 0;
    size_t count = 0;

    for (auto const &tpl : range) {
        if (std::get<0>(tpl) == 0u) {
            sum += std::get<1>(tpl);
            count++;
        }
    }

    v_t const avg   = sum / count;
    v_t       sumsq = 0;
    for (auto const &tpl : range) {
        if (std::get<0>(tpl) == 0u) { sumsq += std::pow((std::get<1>(tpl)) - avg, 2); }
    }

    return (static_cast<double>(sumsq) / count);
}

template <typename T>
requires std::is_arithmetic_v<std::ranges::range_value_t<T>>
auto compute_stdDeviation(T &&range) {
    return (std::sqrt(compute_variance(std::forward<T>(range))));
}


} // namespace incom::standard::numeric