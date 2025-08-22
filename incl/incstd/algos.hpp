#pragma once

#include <algorithm>
#include <functional>
#include <limits>
#include <ranges>
#include <tuple>
#include <type_traits>
#include <utility>

#include <incstd/concepts.hpp>


namespace incom::standard::algos {

constexpr inline std::tuple<double, double> compute_minMaxMulti(auto &&anything) {
    std::pair<double, double> res{std::numeric_limits<double>::max(), std::numeric_limits<double>::min()};

    auto ol_set_solver = [&](this auto const &self, auto const &any) -> void {
        using any_t = std::remove_cvref_t<decltype(any)>;

        if constexpr (incom::standard::concepts::is_some_pair<any_t>) {
            self(any.first);
            self(any.second);
        }
        else if constexpr (incom::standard::concepts::is_some_tuple<any_t>) {
            std::invoke([&]<std::size_t... I>(std::index_sequence<I...>) { (self(std::get<I>), ...); },
                        std::make_index_sequence<std::tuple_size_v<any_t>>{});
        }
        else if constexpr (incom::standard::concepts::is_some_optional<any_t> ||
                           incom::standard::concepts::is_some_expected<any_t>) {
            if (any.has_value()) { self(any.value()); }
        }
        else if constexpr (incom::standard::concepts::is_some_variant<any_t>) { std::visit(self, any); }

        else if constexpr (std::ranges::range<any_t>) {
            using val_type = std::ranges::range_value_t<any_t>;
            if constexpr (std::is_arithmetic_v<val_type>) {
                auto [minV_l, maxV_l] = std::ranges::minmax(any);
                res.first             = std::min(res.first, static_cast<double>(minV_l));
                res.second            = std::max(res.second, static_cast<double>(maxV_l));
            }
            else {
                for (auto const &rngItem : any) { self(rngItem); }
            }
        }
        else if constexpr (std::is_arithmetic_v<any_t>) {
            res.first  = std::min(res.first, static_cast<double>(any));
            res.second = std::max(res.second, static_cast<double>(any));
        }
        else {}
    };

    ol_set_solver(anything);
    return res;
}

template <bool silentSkip = true>
auto compute_deep_applyAndFold(auto const &&onto, auto const &&applier_func, auto const &&fold_func, auto init) {
    using applier_func_t = std::remove_cv_t<decltype(applier_func)>;
    using fold_func_t    = std::remove_cv_t<decltype(fold_func)>;
    using init_t         = std::remove_cv_t<decltype(init)>;

    if constexpr (not std::is_invocable_r_v<init_t, fold_func_t, init_t, init_t>) { static_assert(false); }

    auto ol_set_solver = [&](this auto const &self, auto const &someOnto) -> void {
        using someOnto_t = std::remove_cvref_t<decltype(someOnto)>;

        // When we can invoke applier, we will
        if constexpr (std::is_invocable_r_v<init_t, applier_func_t, someOnto_t>) {
            init = fold_func(init, applier_func(someOnto));
        }

        else if constexpr (incom::standard::concepts::is_some_pair<someOnto_t>) {
            self(someOnto.first);
            self(someOnto.second);
        }
        else if constexpr (incom::standard::concepts::is_some_tuple<someOnto_t>) {
            std::invoke([&]<std::size_t... I>(std::index_sequence<I...>) { (self(std::get<I>), ...); },
                        std::make_index_sequence<std::tuple_size_v<someOnto_t>>{});
        }
        else if constexpr (incom::standard::concepts::is_some_optional<someOnto_t> ||
                           incom::standard::concepts::is_some_expected<someOnto_t>) {
            if (someOnto.has_value()) { self(someOnto.value()); }
        }
        else if constexpr (incom::standard::concepts::is_some_variant<someOnto_t>) { std::visit(self, someOnto); }

        else if constexpr (std::ranges::range<someOnto_t>) {
            for (auto const &rngItem : someOnto) { self(rngItem); }
        }
        else {
            if constexpr (not silentSkip) { static_assert(false); }
        }
    };

    ol_set_solver(onto);
    return init;
}

// Not actual sorting, just give the user sorted indices into original ranges. 
// func_sorterComp operates on tuples of <const & RNGs...> (through projection) but physically sorts tuples of
// <size_t,std::reference_wrapper()...> Outputs sorted indices into the original rngs
template <typename... RNGs>
requires(sizeof...(RNGs) > 0) && (std::ranges::range<RNGs>, ...)
std::vector<size_t> compute_sortedIDXs(auto func_sorterComp, RNGs const &...rngs) {
    auto lam_IS = [&]<size_t... idxs>(std::index_sequence<idxs...>) -> std::vector<size_t> {
        auto filteredVec = std::views::zip(std::views::iota(0uz), rngs...) | std::views::transform([](auto const &tpl) {
                               return std::tuple(std::get<0>(tpl), std::ref(std::get<idxs + 1>(tpl))...);
                           }) |
                           std::ranges::to<std::vector>();

        std::ranges::sort(filteredVec, func_sorterComp,
                          [&](auto const &tpl) { return std::tie(std::get<idxs + 1>(tpl).get()...); });

        return std::views::transform(filteredVec, [](auto const &tpl_item) { return std::get<0>(tpl_item); }) |
               std::ranges::to<std::vector>();
    };

    return lam_IS(std::make_index_sequence<sizeof...(RNGs)>{});
}

// Not actual sorting, just give the user sorted indices into original ranges. 
// func_filter operates on tuples of <size_t, RNGs...>. Means the first element is automatically added to be the index.
// func_sorterComp operates on tuples of <const & RNGs...> (through projection) but physically sorts tuples of <size_t,
// std::reference_wrapper()...> Outputs sorted indices into the original rngs
template <typename... RNGs>
requires(sizeof...(RNGs) > 0) && (std::ranges::range<RNGs>, ...)
std::vector<size_t> compute_filterSortedIDXs(auto func_filter, auto func_sorterComp, RNGs const &...rngs) {
    auto lam_IS = [&]<size_t... idxs>(std::index_sequence<idxs...>) -> std::vector<size_t> {
        auto filteredVec = std::views::zip(std::views::iota(0uz), rngs...) | std::views::filter(func_filter) |
                           std::views::transform([](auto const &tpl) {
                               return std::tuple(std::get<0>(tpl), std::ref(std::get<idxs + 1>(tpl))...);
                           }) |
                           std::ranges::to<std::vector>();

        std::ranges::sort(filteredVec, func_sorterComp,
                          [&](auto const &tpl) { return std::tie(std::get<idxs + 1>(tpl).get()...); });

        return std::views::transform(filteredVec, [](auto const &tpl_item) { return std::get<0>(tpl_item); }) |
               std::ranges::to<std::vector>();
    };

    return lam_IS(std::make_index_sequence<sizeof...(RNGs)>{});
}


} // namespace incom::standard::algos