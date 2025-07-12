#pragma once

#include <iterator>
#include <ranges>
#include <utility>

#include <incstd/typegen.hpp>

namespace incom::standard::views {
namespace detail {
using namespace incom::standard;
using namespace incom::standard::typegen;

template <std::ranges::forward_range RANGE>
struct _kcomb_sentinel {};

template <std::ranges::forward_range RANGE, size_t K>
class _kcomb_iter {
    using base_iterator   = std::ranges::iterator_t<RANGE>;
    using base_sentinel   = std::ranges::sentinel_t<RANGE>;
    using base_value_type = std::ranges::range_value_t<RANGE>;
    using base_reference  = std::ranges::range_reference_t<RANGE>;

    c_generateTuple<K, base_iterator>::type iters;
    c_generateTuple<K, base_sentinel>::type end_iters;

    static constexpr auto idxSeq     = std::make_index_sequence<K>{};
    static constexpr auto idxSeq_rev = transform_integer_sequence<std::views::reverse, decltype(idxSeq)>{};

    template <size_t... I>
    constexpr _kcomb_iter(std::index_sequence<I...>, base_iterator begin, base_sentinel end)
        : iters{(std::next(begin, I))...}, end_iters{std::next(begin, I + (end - begin) - (K - 1))...} {}

public:
    using value_type        = c_generateTuple<K, base_value_type>::type;
    using reference         = c_generateTuple<K, base_reference>::type;
    using difference_type   = std::ptrdiff_t;
    using iterator_category = std::forward_iterator_tag;

    [[nodiscard]] constexpr _kcomb_iter() = default;

    [[nodiscard]] constexpr _kcomb_iter(base_iterator begin, base_sentinel end) : _kcomb_iter(idxSeq, begin, end) {}

    // Prefix increment
    constexpr auto operator++() -> _kcomb_iter & {
        auto lam = [&]<size_t... Is>(std::integer_sequence<size_t, Is...>) -> void {
            auto lamRev = [&]<size_t... Js>(std::integer_sequence<size_t, Js...>) -> void {
                std::array<bool, sizeof...(Is)> idAtLastPos{(Is, false)...};

                if (((std::next(std::get<Js>(iters)) == std::get<Js>(end_iters) ? (idAtLastPos[Js] = true, true)
                                                                                : (std::get<Js>(iters)++, false)) &&
                     ...)) {
                    ((std::get<Is>(iters) = std::get<Is>(end_iters)), ...);
                    return;
                }

                base_iterator *ptr;
                (((idAtLastPos[Is] == true && Is > 0) ? std::get<Is>(iters) = std::next(*ptr),
                  ptr = &(std::get<Is>(iters))        : ptr = &std::get<Is>(iters)),
                 ...);
            };

            lamRev(idxSeq_rev);
        };
        lam(idxSeq);
        return *this;
    }

    // Postfix increment
    [[nodiscard]] constexpr auto operator++(int) -> _kcomb_iter {
        const auto pre = *this;
        ++(*this);
        return pre;
    }

    [[nodiscard]] constexpr auto operator*() const -> reference {
        auto lam = [&]<size_t... I>(std::integer_sequence<size_t, I...>) -> reference {
            return std::tie((*(std::get<I>(iters)))...);
        };
        return lam(idxSeq);
    }

    [[nodiscard]] constexpr auto operator<=>(const _kcomb_iter &) const = default;

    [[nodiscard]] constexpr auto operator==(const _kcomb_sentinel<RANGE> & /*unused*/) const -> bool {
        auto lam = [&]<size_t... I>(std::integer_sequence<size_t, I...> seq) -> bool {
            return ((std::get<I>(iters) == std::get<I>(end_iters) ? (true) : false) && ...);
        };
        return lam(idxSeq);
    }
};

template <std::ranges::forward_range RANGE, size_t K>
requires std::ranges::view<RANGE>
class _kcomb_view : public std::ranges::view_interface<_kcomb_view<RANGE, K>> {
    RANGE base_;

public:
    [[nodiscard]] constexpr _kcomb_view() = default;

    template <size_t KK = K>
    [[nodiscard]] constexpr explicit _kcomb_view(RANGE range) : base_{std::move(range)} {}

    [[nodiscard]] constexpr auto begin() const -> _kcomb_iter<RANGE, K> {
        return _kcomb_iter<RANGE, K>{std::ranges::begin(base_), std::ranges::end(base_)};
    }

    [[nodiscard]] constexpr auto end() const -> _kcomb_sentinel<RANGE> { return {}; }
};

// template <std::ranges::sized_range RANGE>
// combinations_k_view(RANGE &&) -> combinations_k_view<std::views::all_t<RANGE>, 2>;

template <size_t K>
struct _kcomb_fn : std::ranges::range_adaptor_closure<_kcomb_fn<K>> {
    template <typename RANGE>
    constexpr auto operator()(RANGE &&range) const {
        return _kcomb_view<std::views::all_t<RANGE>, K>{std::forward<RANGE>(range)};
    }
};
} // namespace detail

template <size_t K>
constexpr inline detail::_kcomb_fn<K> combinations_k;


} // namespace incom::standard::views