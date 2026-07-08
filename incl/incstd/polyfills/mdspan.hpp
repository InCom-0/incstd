#pragma once

#if defined(INCSTD_USE_BUNDLED_MDSPAN) && defined(INCSTD_USE_BUNDLED_SUBMDSPAN)

#if INCSTD_USE_BUNDLED_SUBMDSPAN

#if INCSTD_USE_BUNDLED_MDSPAN

#include <experimental/mdspan>

#else
#warning Including mdspan polyfill under Kokkos:: namespace because partial (without submdspan) implementation exists on your system and it would conflict otherwise
#include <mdspan/mdspan.hpp>

#ifndef INCSTD_MDSPAN_UNDER_KOKKOS
#define INCSTD_MDSPAN_UNDER_KOKKOS
#endif
#endif

#else
#include <mdspan>
#endif

#else
#error incstd: mdspan polyfill header is being included without the required definitions.
#endif