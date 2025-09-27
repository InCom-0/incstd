#pragma once

#include <string_view>


namespace incom::standard::console {
enum class err_terminal {
    NoTerminal = 1,
    IoError,
    Timeout,
    ParseError,
    Unsupported
};

[[nodiscard]] inline constexpr std::string_view to_string(err_terminal e) noexcept {
    using namespace std::literals;
    switch (e) {
        case err_terminal::NoTerminal:  return "NoTerminal"sv;
        case err_terminal::IoError:     return "IoError"sv;
        case err_terminal::Timeout:     return "Timeout"sv;
        case err_terminal::ParseError:  return "ParseError"sv;
        case err_terminal::Unsupported: return "Unsupported"sv;
    }
    return "Unknown"sv;
}


} // namespace incom::standard::console
