#pragma once

namespace incom::standard::pack {
using namespace incom::standard;


template <typename... T>
struct utility {
    template <typename... Ts_toPass_toEachCTOR>
    static void invoke_overCTORed(auto const &&func, Ts_toPass_toEachCTOR const &...ttptc) {
        (func(T{ttptc...}), ...);
    };
};
} // namespace incom::standard::pack