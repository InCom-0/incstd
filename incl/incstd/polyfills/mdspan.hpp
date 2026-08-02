#pragma once

#include <version>


#if defined(INCSTD_MDSPAN_FORCE_BUNDLED) // FORCE BUNDLED
#if defined(__cpp_lib_mdspan) && (__cpp_lib_mdspan >= 202207L)
#warning Including mdspan polyfill under Kokkos:: namespace because partial (without submdspan) implementation exists on your system and it would conflict otherwise
#include <mdspan/mdspan.hpp>
#ifndef INCSTD_MDSPAN_UNDER_KOKKOS
#define INCSTD_MDSPAN_UNDER_KOKKOS
#endif

#else
#include <experimental/mdspan>
#endif


#elif defined(INCSTD_MDSPAN_FORCE_STDLIB) // FORCE STDLIB
#if ! defined(__cpp_lib_mdspan) || (__cpp_lib_mdspan < 202207L) || ! defined(__cpp_lib_submdspan) ||                   \
    (__cpp_lib_submdspan < 202306L)
#error "INCSTD_MDSPAN_FORCE_STDLIB is set, but stdlib lacks required mdspan/submdspan."

#else
#include <mdspan>
#endif


#else // AUTO
#if ! defined(__cpp_lib_mdspan) || (__cpp_lib_mdspan < 202207L)
#include <experimental/mdspan>

#elif defined(__cpp_lib_mdspan) && (__cpp_lib_mdspan >= 202207L) && defined(__cpp_lib_submdspan) &&                    \
    (__cpp_lib_submdspan >= 202306L)
#include <mdspan>

#else
#warning Including mdspan polyfill under Kokkos:: namespace because partial (without submdspan) implementation exists on your system and it would conflict otherwise
#include <mdspan/mdspan.hpp>
#ifndef INCSTD_MDSPAN_UNDER_KOKKOS
#define INCSTD_MDSPAN_UNDER_KOKKOS
#endif
#endif
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