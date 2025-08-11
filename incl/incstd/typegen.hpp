#pragma once

#include <concepts>
#include <ranges>
#include <utility>
#include <typeinfo>


namespace incom::standard::typegen {
using namespace incom::standard;

// Generating a tuple with of N times the supplied type
// Usage: pass the N parameter and the T type only, leave the ...Ts pack empty
// Note: If you don't adhere to the above the output tuple will have more 'member types' than you want. Beware.
// Note: Uses recursive template instantiation.
// Note: T and Ts all must be the same types.
template <std::size_t N, typename T, typename... Ts>
requires(std::same_as<T, Ts> && ...)
struct c_generateTuple {
    using type = typename c_generateTuple<N - 1, T, T, Ts...>::type;
};

template <typename T, typename... Ts>
requires(std::same_as<T, Ts> && ...)
struct c_generateTuple<0, T, Ts...> {
    using type = std::tuple<Ts...>;
};

// Generating an 'integer_sequence' of N identical size_t integers of value S.
// Usage: pass the N parameter and the S integer only, leave the ...Sx pack empty
// Note: If you don't adhere to the above the output integer_sequence will be longer than you (probably) want. Beware.
// Note: Uses recursive template instantiation.
// Note: S and Sx must all be the same value.
template <std::size_t N, long long S, long long... Sx>
requires((S == Sx) && ...)
struct c_gen_X_repeat_sequence {
    using type = typename c_gen_X_repeat_sequence<N - 1, S, S, Sx...>::type;
};
template <long long S, long long... Sx>
requires((S == Sx) && ...)
struct c_gen_X_repeat_sequence<0, S, Sx...> {
    using type = std::integer_sequence<long long, Sx...>;
};

namespace detail {
// Shamelessly 'borrowed' from https://godbolt.org/z/6nbfsbTz1 ... from a comment on SO
// https://stackoverflow.com/questions/72706224/how-do-i-reverse-the-order-of-the-integers-in-a-stdinteger-sequenceint-4

template <auto View, typename INT, INT... Is>
auto build_sequence(std::integer_sequence<INT, Is...>) {
    // We build an array holding the input sequence of integers
    constexpr std::array<INT, sizeof...(Is)> input{Is...};

    constexpr decltype(View) View2 = View;

    // We get the size of the output sequence of integers
    constexpr auto N = std::size(std::views::iota(0uz, sizeof...(Is)) | View);

    // We build the array holding the output sequence of integers
    constexpr std::array<INT, N> values = [&] {
        std::array<INT, N> res;
        auto               vw = std::views::iota(size_t{0}, sizeof...(Is)) | View2;
        for (size_t i = 0; auto x : vw) { res.at(i++) = input.at(x); }
        return res;
    }();

    // We build the output std::integer_sequence object (with a different pack Os...)
    return [&]<std::size_t... Os>(std::index_sequence<Os...>) {
        return std::integer_sequence<INT, values[Os]...>{};
    }(std::make_index_sequence<N>());
}
} // namespace detail

// We define an alias that builds the std::integer_sequence result from a call to build_sequence
template <auto view, typename Sequence>
using transform_integer_sequence = decltype(detail::build_sequence<view>(Sequence{}));


} // namespace incom::standard::typegen