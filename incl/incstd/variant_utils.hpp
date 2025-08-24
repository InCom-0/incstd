#pragma once

#include <utility>
#include <unordered_map>

#include <incstd/concepts.hpp>
#include <incstd/typegen.hpp>


namespace incom::standard::variant_utils {
using namespace incom::standard;

template <typename... Ts>
struct VariantUtility {
    // PTC = Types To Pass To Constructors
    template <typename... PTC>
    static constexpr inline auto gen_typeMap(PTC const &...ptc) {
        std::unordered_map<std::type_index, const std::variant<Ts...>> res;
        (res.insert({typegen::get_typeIndex<Ts>(), std::variant<Ts...>(Ts(std::forward<decltype(ptc)>(ptc)...))}), ...);
        return res;
    };
};


} // namespace incom::standard::variantUtils