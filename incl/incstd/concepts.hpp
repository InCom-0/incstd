#pragma once

#include <concepts>
#include <type_traits>
#include <utility>
#include <optional>
#include <expected>
#include <variant>


namespace incom::standard::concepts {
using namespace incom::standard;

namespace detail {
template <typename T, template <typename...> typename Template>
struct _is_specialization_of : std::false_type {};
template <template <typename...> typename Template, typename... Args>
struct _is_specialization_of<Template<Args...>, Template> : std::true_type {};
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


} // namespace incom::standard::concepts