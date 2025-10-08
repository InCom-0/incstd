#pragma once

#include <string_view>

#if defined(__MINGW64__)
#include <unistd.h>
#include <windows.h>

#elif defined(_WIN64)
#include <io.h>
#include <windows.h>

#elif defined(__linux__) || (defined(__APPLE__) && defined(__MACH__))
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#endif


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


inline std::pair<int, int> get_rowColCount() {
#if defined(_WIN64)
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);

    return std::make_pair(static_cast<int>(csbi.dwSize.Y), static_cast<int>(csbi.dwSize.X));

#elif defined(__linux__) || (defined(__APPLE__) && defined(__MACH__))
    winsize ws;

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0) {
        if (ws.ws_row == 0 || ws.ws_col == 0) { return std::make_pair(0, 0); }
        else { return std::make_pair(static_cast<int>(ws.ws_row), static_cast<int>(ws.ws_col)); }
    }
    else { return std::make_pair(0, 0); }
#endif
}

inline bool is_inTerminal() {
    auto sizePair = get_rowColCount();
    if (sizePair.first < 1 || sizePair.second < 1) { return false; }
    else { return true; }
}

inline bool is_stdin_inTerminal() {
#if defined(__linux__) || defined(__MINGW64__) || (defined(__APPLE__) && defined(__MACH__))
    return isatty(fileno(stdin));
#elif defined(_WIN64)
    return _isatty(_fileno(stdin));
#endif
};

inline bool is_stdout_inTerminal() {
#if defined(__linux__) || defined(__MINGW64__) || (defined(__APPLE__) && defined(__MACH__))
    return isatty(fileno(stdout));
#elif defined(_WIN64)
    return _isatty(_fileno(stdout));
#endif
};

inline void set_cocp() {
#if defined(_WIN64)
    SetConsoleOutputCP(CP_UTF8);
#elif defined(__linux__) || (defined(__APPLE__) && defined(__MACH__))
#endif
}


} // namespace incom::standard::console
