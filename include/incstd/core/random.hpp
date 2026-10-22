#pragma once

#include <cstdint>
#include <limits>

#if defined(_MSC_VER) && ! defined(__clang__)
#include <intrin.h>
#endif

namespace incom::standard::random {

struct FastPseudoRandom {
    std::uint64_t m_state = 0x9e3779b97f4a7c15ull;

    FastPseudoRandom() = default;

    explicit FastPseudoRandom(std::uint64_t seed) noexcept : m_state(seed) {}

    void
    setSeed(std::uint64_t seed) noexcept {
        m_state = seed;
    }

    [[nodiscard]] std::uint64_t
    nextRandomWord() noexcept {
        std::uint64_t z = (m_state += 0x9e3779b97f4a7c15ull);
        z               = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ull;
        z               = (z ^ (z >> 27)) * 0x94d049bb133111ebull;
        return z ^ (z >> 31);
    }

    [[nodiscard]] static std::uint64_t
    multiplyHigh64(std::uint64_t lhs, std::uint64_t rhs) noexcept {
#if defined(_MSC_VER) && ! defined(__clang__)
        return __umulh(lhs, rhs);
#else
        return static_cast<std::uint64_t>((static_cast<unsigned __int128>(lhs) * rhs) >> 64);
#endif
    }

    [[nodiscard]] std::size_t
    pseudoRandom_0_to(std::size_t maxInclusive) noexcept {
        if (maxInclusive == std::numeric_limits<std::size_t>::max()) {
            return static_cast<std::size_t>(nextRandomWord());
        }

        auto const bound     = static_cast<std::uint64_t>(maxInclusive) + 1ull;
        auto const threshold = (0ull - bound) % bound;

        for (;;) {
            auto const randomWord = nextRandomWord();

#if defined(_MSC_VER) && ! defined(__clang__)
            auto const low  = randomWord * bound;
            auto const high = __umulh(randomWord, bound);
#else
            auto const product = static_cast<unsigned __int128>(randomWord) * bound;
            auto const low     = static_cast<std::uint64_t>(product);
            auto const high    = static_cast<std::uint64_t>(product >> 64);
#endif

            if (low >= threshold) { return static_cast<std::size_t>(high); }
        }
    }
};

} // namespace incom::standard::random