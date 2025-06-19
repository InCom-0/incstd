#pragma once

#include <concepts>
#include <utility>

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

} // namespace incom::standard::typegen