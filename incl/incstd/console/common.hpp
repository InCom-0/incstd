#pragma once


namespace incom::standard::console {

enum class err_terminal {
    NoTerminal = 1,
    IoError,
    Timeout,
    ParseError,
    Unsupported
};


} // namespace incom::standard::console
