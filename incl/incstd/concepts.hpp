#pragma once

#include <concepts>
#include <expected>
#include <optional>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>
#include <incstd/views.hpp>



namespace incom::standard::concepts {
using namespace incom::standard;

namespace detail {
template <typename T, template <typename...> typename Template>
struct _is_specialization_of : std::false_type {};
template <template <typename...> typename Template, typename... Args>
struct _is_specialization_of<Template<Args...>, Template> : std::true_type {};

template <typename... Ts>
struct _types_noneSame {
    static consteval bool operator()() {
        std::vector<const std::type_info *> typeVect{&typeid(Ts)...};
        for (auto [lhs, rhs] : incom::standard::views::combinations_k<2uz>(typeVect)) {
            if (*lhs == *rhs) { return false; }
        }
        return true;
    }
};
} // namespace detail


// Note: SpecializationOf does not support non-type template parameteres (at all) ... beware
template <typename T, template <typename...> typename Template>
concept is_specialization_of = detail::_is_specialization_of<T, Template>::value;

template <std::size_t N>
concept is_power_of2 = ((N != 0) && ! (N & (N - 1)));

template <typename T>
concept has_size_mf = requires {
    typename T::size_type;
    { std::declval<T>().size() } -> std::same_as<typename T::size_type>;
};

template <typename T>
concept is_some_pair = is_specialization_of<T, std::pair>;
template <typename T>
concept is_some_tuple = is_specialization_of<T, std::tuple>;


template <typename T>
concept is_some_optional = is_specialization_of<T, std::optional>;
template <typename T>
concept is_some_expected = is_specialization_of<T, std::expected>;
template <typename T>
concept is_some_variant = is_specialization_of<T, std::variant>;


// Just the 'last level' type name ... not the fully qualified typename
template <typename... Ts>
concept types_noneSame_v = detail::_types_noneSame<Ts...>::operator()();


} // namespace incom::standard::concepts