#pragma once

#include <array>
#include <ranges>
#include <tuple>
#include <utility>

#include <incstd/core/typegen.hpp>

namespace incom::standard::views {
namespace detail {
using namespace incom::standard;
using namespace incom::standard::typegen;

template <std::ranges::forward_range RANGE>
struct _kcomb_sentinel {};

template <std::ranges::forward_range RANGE, size_t K>
requires(K > 1)
class _kcomb_iter {
    using base_iterator   = std::ranges::iterator_t<RANGE>;
    using base_sentinel   = std::ranges::sentinel_t<RANGE>;
    using base_difference = std::ranges::range_difference_t<RANGE>;
    using base_value_type = std::ranges::range_value_t<RANGE>;
    using base_reference  = std::ranges::range_reference_t<RANGE>;

    std::array<base_iterator, K> iters;
    std::array<base_iterator, K> end_iters;

    static constexpr auto k_as_diff = static_cast<base_difference>(K);

    static constexpr auto idxSeq = std::make_index_sequence<K>{};

public:
    using value_type        = c_generateTuple<K, base_value_type>::type;
    using reference         = c_generateTuple<K, base_reference>::type;
    using difference_type   = std::ptrdiff_t;
    using iterator_category = std::forward_iterator_tag;

    [[nodiscard]] constexpr _kcomb_iter() = default;

    [[nodiscard]] constexpr _kcomb_iter(base_iterator begin, base_sentinel end) {
        const auto end_it = std::ranges::next(begin, end);
        const auto n      = std::ranges::distance(begin, end);

        // No valid K-combination exists when the range is shorter than K.
        if (n < k_as_diff) {
            iters.fill(end_it);
            end_iters.fill(end_it);
            return;
        }

        for (size_t i = 0; i < K; ++i) {
            const auto idx = static_cast<base_difference>(i);
            iters[i]       = std::ranges::next(begin, idx, end);
            end_iters[i]   = std::ranges::next(begin, idx + (n - (k_as_diff - static_cast<base_difference>(1))), end);
        }
    }


    // TODO: Explore possibility of turning it into a coroutine somehow
    //  Prefix increment
    constexpr auto
    operator++() -> _kcomb_iter & {
        for (size_t i = K; i-- > 0;) {
            auto next_it = iters[i];
            ++next_it;
            if (next_it == end_iters[i]) { continue; }

            ++iters[i];
            for (size_t j = i + 1; j < K; ++j) {
                iters[j] = iters[j - 1];
                ++iters[j];
            }
            return *this;
        }

        // Mark as exhausted.
        iters = end_iters;
        return *this;
    }

    // Postfix increment
    [[nodiscard]] constexpr auto
    operator++(int) -> _kcomb_iter {
        const auto pre = *this;
        ++(*this);
        return pre;
    }

    [[nodiscard]] constexpr auto
    operator*() const -> reference {
        auto lam = [&]<size_t... Is>(std::integer_sequence<size_t, Is...>) -> reference {
            return std::tie(*iters[Is]...);
        };
        return lam(idxSeq);
    }

    [[nodiscard]] constexpr auto
    operator==(const _kcomb_iter &) const -> bool = default;

    [[nodiscard]] constexpr auto
    operator==(const _kcomb_sentinel<RANGE> & /*unused*/) const -> bool {
        return iters[K - 1] == end_iters[K - 1];
    }
};

template <std::ranges::forward_range RANGE, size_t K>
requires std::ranges::view<RANGE> && (K > 1)
class _kcomb_view : public std::ranges::view_interface<_kcomb_view<RANGE, K>> {
    RANGE base_;

public:
    [[nodiscard]] constexpr _kcomb_view() = default;

    [[nodiscard]] constexpr explicit _kcomb_view(RANGE range) : base_{std::move(range)} {}

    [[nodiscard]] constexpr auto
    begin() -> _kcomb_iter<RANGE, K> {
        return _kcomb_iter<RANGE, K>{std::ranges::begin(base_), std::ranges::end(base_)};
    }

    [[nodiscard]] constexpr auto
    end() -> _kcomb_sentinel<RANGE> {
        return {};
    }

    [[nodiscard]] constexpr auto
    begin() const -> _kcomb_iter<const RANGE, K>
    requires std::ranges::forward_range<const RANGE>
    {
        return _kcomb_iter<const RANGE, K>{std::ranges::begin(base_), std::ranges::end(base_)};
    }

    [[nodiscard]] constexpr auto
    end() const -> _kcomb_sentinel<const RANGE>
    requires std::ranges::forward_range<const RANGE>
    {
        return {};
    }
};

// template <std::ranges::sized_range RANGE>
// combinations_k_view(RANGE &&) -> combinations_k_view<std::views::all_t<RANGE>, 2>;

template <size_t K>
struct _kcomb_fn : std::ranges::range_adaptor_closure<_kcomb_fn<K>> {
    template <std::ranges::viewable_range RANGE>
    requires std::ranges::forward_range<RANGE>
    constexpr auto
    operator()(RANGE &&range) const {
        return _kcomb_view<std::views::all_t<RANGE>, K>{std::views::all(std::forward<RANGE>(range))};
    }
};
} // namespace detail

template <size_t K>
requires(K > 1)
constexpr inline detail::_kcomb_fn<K> combinations_k;


} // namespace incom::standard::views