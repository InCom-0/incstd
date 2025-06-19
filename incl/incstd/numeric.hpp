#pragma once

#include <cmath>
#include <cstdlib>
#include <type_traits>


namespace incom::standard::numeric {
using namespace incom::standard;

template <typename T>
requires std::is_integral_v<T>
int get_numOfDigits(T const &integralNumber) {
    if constexpr (std::is_unsigned_v<T>) { return integralNumber == 0 ? 1 : std::trunc(log10(integralNumber)) + 1; }
    else { return integralNumber == 0 ? 1 : std::trunc(log10(std::abs(integralNumber))) + 1; }
}

}