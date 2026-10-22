#pragma once

#ifndef INCSTD_INCLUDING_BLACKBIRD_COMBINATORS
#define INCSTD_INCLUDING_BLACKBIRD_COMBINATORS

#ifdef combinators
#error                                                                                                                 \
    "Macro conflict. 'combinators' cannot be defined at this point. This shouldn't ever happen unless conflict with some other library."

#else
#define combinators incom::standard::combinators::__detail__
#include <incstd/_vendor/blackbird/blackbird_combinators.hpp>
#undef combinators

#endif
#undef INCSTD_INCLUDING_BLACKBIRD_COMBINATORS
#endif

namespace incom::standard::combinators {
using namespace ::incom::standard::combinators::__detail__;

} // namespace incom::standard::combinators
