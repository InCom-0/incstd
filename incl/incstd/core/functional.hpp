#pragma once

#include <incstd/core/views.hpp>


namespace incom::standard::functional {

template <auto Value>
struct constant_t {
    static constexpr auto
    operator()(auto &&...) noexcept {
        return Value;
    }
};

using always_true_t  = constant_t<true>;
using always_false_t = constant_t<false>;

struct no_op_t {
    static constexpr void
    operator()(auto &&...) noexcept {}
};

inline constexpr always_true_t  always_true{};
inline constexpr always_false_t always_false{};
inline constexpr no_op_t        no_op{};


using namespace incom::standard;
} // namespace incom::standard::functional