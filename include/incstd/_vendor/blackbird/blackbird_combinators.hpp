/* MIT License

Copyright (c) 2022 Conor Hoekstra

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE. */

#pragma once

#if ! defined(INCSTD_INCLUDING_BLACKBIRD_COMBINATORS)
#if ! defined(__INTELLISENSE__) && ! defined(__clangd__) && ! defined(__CLANGD__)
#error                                                                                                                 \
    "This vendored include file cannot be included in consuming projects directly. Please #include <incstd/core/combinators.hpp> instead"
#endif
#endif

#include <algorithm>
#include <cmath>
#include <functional>

namespace combinators {

/////////////////
// combinators //
/////////////////

// I (no bird name)
inline auto _id = std::identity{};

// B (The Bluebird)
inline auto _b = [](auto f, auto g) { return [=](auto x) { return f(g(x)); }; };

// BB (The Bluebird^2)
inline auto _bb = [](auto f, auto g, auto h) { return [=](auto x) { return f(g(h(x))); }; };

// BBB (The Bluebird^3)
inline auto _bbb = [](auto f, auto g, auto h, auto i) { return [=](auto x) { return f(g(h(i(x)))); }; };

// B1 (The Blackbird)
inline auto _b1 = [](auto f, auto g) { return [=](auto x, auto y) { return f(g(x, y)); }; };

// C (The Cardinal) aka `flip` in Haskell
inline auto _c = [](auto f) { return [=](auto x, auto y) { return f(y, x); }; };

// D2 (The Dovekie)
inline auto _d2_ = [](auto f, auto g, auto h) { return [=](auto x, auto y) { return g(f(x), h(y)); }; };

// K (Kestrel)
inline auto _l_ = [](auto x, auto y) { return x; };

// KI
inline auto _r_ = [](auto x, auto y) { return y; };

// Phi (The Phoenix)
inline auto _phi = [](auto f, auto g, auto h) { return [=](auto x) { return g(f(x), h(x)); }; };

// Phi1 (The Pheasant)
inline auto _phi1_ = [](auto f, auto g, auto h) { return [=](auto x, auto y) { return g(f(x, y), h(x, y)); }; };

// Psi (The Psi Bird)
inline auto _psi = [](auto f, auto g) { return [=](auto x, auto y) { return f(g(x), g(y)); }; };

// W (The Warbler)
inline auto _w = [](auto f) { return [=](auto x) { return f(x, x); }; };

/////////////////////////////////////////////
// more convenient binary/unary operations //
/////////////////////////////////////////////

inline auto _eq    = [](auto x) { return [x](auto y) { return x == y; }; };
inline auto _eq_   = std::equal_to{};
inline auto _neq_  = std::not_equal_to{};
inline auto _lt    = [](auto x) { return [x](auto y) { return x > y; }; };
inline auto lt_    = [](auto x) { return [x](auto y) { return y < x; }; };
inline auto _lt_   = std::less{};
inline auto _lte_  = std::less_equal{};
inline auto _gt_   = std::greater{};
inline auto _gte   = [](auto x) { return [x](auto y) { return x >= y; }; };
inline auto _gte_  = std::greater_equal{};
inline auto _plus  = [](auto x) { return [x](auto y) { return x + y; }; };
inline auto _plus_ = std::plus{};
inline auto _mul   = [](auto x) { return [x](auto y) { return x * y; }; };
inline auto _mul_  = std::multiplies{};
inline auto _sub   = [](auto x) { return [x](auto y) { return x - y; }; };
inline auto sub_   = [](auto x) { return [x](auto y) { return y - x; }; };
inline auto _sub_  = std::minus{};
inline auto _mod   = [](auto x) { return [x](auto y) { return x % y; }; };
inline auto mod_   = [](auto x) { return [x](auto y) { return y % x; }; };
inline auto _mod_  = std::modulus{};
inline auto _pow_  = [](auto x, auto y) { return std::pow(x, y); };
inline auto _div   = [](auto x) { return [x](auto y) { return x / y; }; };
inline auto div_   = [](auto x) { return [x](auto y) { return y / x; }; };
inline auto _div_  = std::divides{};
inline auto _or_   = std::logical_or{};
inline auto _and_  = std::logical_and{};
inline auto _neg   = std::negate{};
inline auto _not   = std::logical_not{};
inline auto _sign  = [](auto x) { return x > 0 ? 1 : x < 0 ? -1 : 0; };
inline auto _sqrt  = [](auto x) { return std::sqrt(x); };
inline auto _abs   = [](auto x) { return std::abs(x); };
inline auto _exp   = [](auto x) { return std::exp(x); };
inline auto _log   = [](auto x) { return std::log(x); };
inline auto _sq    = [](auto x) { return x * x; };
inline auto _min_  = [](auto x, auto y) { return std::min(x, y); };
inline auto _max_  = [](auto x, auto y) { return std::max(x, y); };

// conversions
inline auto _int    = [](auto x) { return static_cast<int>(x); };
inline auto _uint   = [](auto x) { return static_cast<unsigned int>(x); };
inline auto _bool   = [](auto x) { return static_cast<bool>(x); };
inline auto _float  = [](auto x) { return static_cast<float>(x); };
inline auto _double = [](auto x) { return static_cast<double>(x); };

template <int N>
inline auto _nth = [](auto t) { return std::get<N>(t); };
inline auto _fst = [](auto t) { return std::get<0>(t); };
inline auto _snd = [](auto t) { return std::get<1>(t); };

inline auto _rshift_  = [](auto x, auto y) { return x >> y; };
inline auto _bit_and_ = std::bit_and{};
inline auto _bit_xor_ = std::bit_xor{};

inline auto _odd  = [](auto x) { return (x % 2) == 1; };
inline auto _even = [](auto x) { return (x % 2) == 0; };

} // namespace combinators
