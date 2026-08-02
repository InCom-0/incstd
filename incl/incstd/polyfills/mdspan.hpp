#pragma once

#include <version>

#if __cpp_lib_mdspan >= 202207L
#if __cpp_lib_submdspan >= 202306L
#include <mdspan>

#else
#warning Including mdspan polyfill under Kokkos:: namespace because partial (without submdspan) implementation exists on your system and it would conflict otherwise
#include <mdspan/mdspan.hpp>

#ifndef INCSTD_MDSPAN_UNDER_KOKKOS
#define INCSTD_MDSPAN_UNDER_KOKKOS
#endif
#endif

#else
#include <experimental/mdspan>
#endif


namespace incom::standard::polyfills {
#if defined(INCSTD_MDSPAN_UNDER_KOKKOS)
template <class IndexType, size_t Rank>
using dextents = Kokkos::dextents<IndexType, Rank>;

template <class ElementType, class Extents>
using mdspan = Kokkos::mdspan<ElementType, Extents>;

template <class... Args>
static constexpr decltype(auto)
submdspan(Args &&...args) {
    return Kokkos::submdspan(std::forward<Args>(args)...);
}

#else
template <class IndexType, size_t Rank>
using dextents = std::dextents<IndexType, Rank>;

template <class ElementType, class Extents>
using mdspan = std::mdspan<ElementType, Extents>;

template <class... Args>
static constexpr decltype(auto)
submdspan(Args &&...args) {
    return std::submdspan(std::forward<Args>(args)...);
}
#endif
} // namespace incom::standard::polyfills