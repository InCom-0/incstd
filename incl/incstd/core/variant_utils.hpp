#pragma once

#include <unordered_map>

#include <incstd/core/concepts.hpp>
#include <incstd/core/typegen.hpp>


namespace incom::standard::variant_utils {
using namespace incom::standard;

// ...Ts are std::variant alternatives
template <typename... Ts>
requires concepts::types_noneSame_v<Ts...>
struct VariantUtility {
    // Constructs a map of variants instantiated with individual variant alternatives, keyed on std::type_index of the
    // alternative inside
    // Very useful in 'prototype' pattern going from 'value land' into 'type land'
    // PTC = Pass To Constructors
    template <typename... PTC>
    static constexpr auto gen_alternsMap(PTC const &...ptc) {
        return std::unordered_map<std::type_index, const std::variant<Ts...>>{
            {typegen::get_typeIndex<Ts>(), std::variant<Ts...>(Ts(std::forward<decltype(ptc)>(ptc)...))}...};
    };
};


} // namespace incom::standard::variant_utils