#pragma once

#include <concepts>
#include <type_traits>


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

template <typename T>
concept is_some_pair = requires(std::remove_cvref_t<T> pair) {
    { pair.first } -> std::same_as<typename T::first_type &>;
    { pair.second } -> std::same_as<typename T::second_type &>;
};
template <std::size_t N>
concept is_power_of2 = ((N != 0) && ! (N & (N - 1)));
} // namespace incom::standard::concepts