#pragma once
#include <array>
#include <charconv>
#include <chrono>
#include <cstdint>
#include <expected>
#include <regex>
#include <string>
#include <string_view>

#ifdef _WIN32
#include <windows.h>
#else
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#endif

#include "colorschemes.hpp"
#include "console_common.hpp"


namespace incom::standard::console {
using namespace incom::standard::color;

class ColorQuery {
public:
    // ────────────── PUBLIC API ──────────────
    using Result = std::expected<inc_sRGB, err_terminal>;

    // query palette index 0..255 (returns expected)
    [[nodiscard]] static constexpr Result get_paletteIdx(int index) {
#ifdef _WIN32
        return get_defaultColor(index);
#else
        return queryPaletteIndex(index);
#endif
    }

    // convenience: always returns something (default palette on failure)
    [[nodiscard]] static constexpr inc_sRGB get_paletteIdx_fb(int index) noexcept {
#ifdef _WIN32
        return get_defaultColor(index);
#else
        auto res = queryPaletteIndex(index);
        return res ? *res : get_defaultColor(index);
#endif
    }

    // foreground
    [[nodiscard]] static constexpr Result get_foreground() {
#ifdef _WIN32
        return get_defaultColor(7);
#else
        return queryForeground();
#endif
    }

    [[nodiscard]] static constexpr inc_sRGB get_foreground_fb() noexcept {
#ifdef _WIN32
        return get_defaultColor(7);
#else
        auto res = queryForeground();
        return res ? *res : get_defaultColor(7);
#endif
    }

    // background
    [[nodiscard]] static constexpr Result get_background() {
#ifdef _WIN32
        return get_defaultColor(0);
#else
        return queryBackground();
#endif
    }

    [[nodiscard]] static constexpr inc_sRGB get_background_fb() noexcept {
#ifdef _WIN32
        return get_defaultColor(0);
#else
        auto res = queryBackground();
        return res ? *res : get_defaultColor(0);
#endif
    }

    // get all 16 colors at once
    [[nodiscard]] static constexpr std::expected<palette16, err_terminal> get_palette16() noexcept {
        palette16 colors{};
        for (int i = 0; i < std::tuple_size_v<decltype(colors)>; ++i) {
#ifdef _WIN32
            colors[i] = get_defaultColor(i);
#else
            auto res = queryPaletteIndex(i);
            if (res.has_value()) { colors[i] = res.value(); }
            else { return std::unexpected(res.error()); }
#endif
        }
        return colors;
    }

    // get all 16 colors at once with fallback
    [[nodiscard]] static constexpr std::expected<palette16, err_terminal> get_palette16_fb() noexcept {
        palette16 colors{};
        for (int i = 0; i < std::tuple_size_v<decltype(colors)>; ++i) {
#ifdef _WIN32
            colors[i] = get_defaultColor(i);
#else
            auto res = queryPaletteIndex(i);
            if (res.has_value()) { colors[i] = res.value(); }
            else { colors[i] = get_defaultColor(i); }
#endif
        }
        return colors;
    }

    // get all 256 colors at once
    [[nodiscard]] static constexpr std::expected<palette256, err_terminal> get_palette256() noexcept {
        palette256 colors{};
        for (int i = 0; i < std::tuple_size_v<decltype(colors)>; ++i) {
#ifdef _WIN32
            colors[i] = get_defaultColor(i);
#else
            auto res = queryPaletteIndex(i);
            if (res.has_value()) { colors[i] = res.value(); }
            else { return std::unexpected(res.error()); }
#endif
        }
        return colors;
    }

    // get all 256 colors at once
    [[nodiscard]] static constexpr std::expected<palette256, err_terminal> get_palette256_TMP() noexcept {
        palette256 colors{};
        for (int i = 0; i < std::tuple_size_v<decltype(colors)>; ++i) {
            auto res = queryPaletteIndex(i);
            if (res.has_value()) { colors[i] = res.value(); }
            else { return std::unexpected(res.error()); }
        }
        return colors;
    }

    // get all 256 colors at once with fallback
    [[nodiscard]] static constexpr std::expected<palette256, err_terminal> get_palette256_fb() noexcept {
        palette256 colors{};
        for (int i = 0; i < std::tuple_size_v<decltype(colors)>; ++i) {
#ifdef _WIN32
            colors[i] = get_defaultColor(i);
#else
            auto res = queryPaletteIndex(i);
            if (res.has_value()) { colors[i] = res.value(); }
            else { colors[i] = get_defaultColor(i); }
#endif
        }
        return colors;
    }

    // Get color from the 'default' palette
    [[nodiscard]] static constexpr inc_sRGB get_defaultColor(int index) noexcept {
        if (! index16_valid(index)) { return inc_sRGB{255, 255, 255}; }
        return color_schemes::defaultScheme16.palette[index];
    }

private:
    // ────────────── INTERNAL ──────────────

    [[nodiscard]] static constexpr bool index256_valid(int idx) noexcept { return idx >= 0 && idx <= 255; }
    [[nodiscard]] static constexpr bool index16_valid(int idx) noexcept { return idx >= 0 && idx <= 15; }

#ifdef _WIN32
    // So far nothing here
    // In the future might have some implementation once Windows terminal reports actually used colors with OSC 4


    // RAII wrapper for HANDLE
    struct uniq_handle {
        HANDLE h{INVALID_HANDLE_VALUE};
        explicit uniq_handle(HANDLE hh) : h(hh) {}
        ~uniq_handle() {
            if (h != INVALID_HANDLE_VALUE && h != nullptr) { CloseHandle(h); }
        }
        uniq_handle(const uniq_handle &)            = delete;
        uniq_handle &operator=(const uniq_handle &) = delete;
        uniq_handle(uniq_handle &&o) noexcept : h(o.h) { o.h = INVALID_HANDLE_VALUE; }
        uniq_handle &operator=(uniq_handle &&o) noexcept {
            if (h != INVALID_HANDLE_VALUE && h != nullptr) { CloseHandle(h); }
            h   = o.h;
            o.h = INVALID_HANDLE_VALUE;
            return *this;
        }
        bool   valid() const noexcept { return h != INVALID_HANDLE_VALUE && h != nullptr; }
        HANDLE get() const noexcept { return h; }
    };

    // Save/restore console input mode (for CONIN). Not constexpr.
    struct console_mode_guard {
        HANDLE h{INVALID_HANDLE_VALUE};
        DWORD  oldMode{0};
        bool   active{false};
        explicit console_mode_guard(HANDLE hh) : h(hh) {
            if (h != INVALID_HANDLE_VALUE && h != nullptr) {
                DWORD mode;
                if (GetConsoleMode(h, &mode)) {
                    oldMode    = mode;
                    // turn off line input and echo to read raw bytes like POSIX
                    DWORD raw  = mode;
                    raw       &= ~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT);
                    // Keep processed input (ENABLE_PROCESSED_INPUT) so Ctrl-C still works as usual.
                    if (SetConsoleMode(h, raw)) { active = true; }
                }
            }
        }
        ~console_mode_guard() {
            if (active) { SetConsoleMode(h, oldMode); }
        }
        console_mode_guard(const console_mode_guard &)            = delete;
        console_mode_guard &operator=(const console_mode_guard &) = delete;
    };

    // write_all for HANDLE using WriteFile
    static bool write_all_handle(HANDLE h, const char *data, size_t len) noexcept {
        if (h == INVALID_HANDLE_VALUE || h == nullptr) { return false; }
        size_t done = 0;
        while (done < len) {
            DWORD written = 0;
            BOOL  ok      = WriteFile(h, data + done, static_cast<DWORD>(len - done), &written, nullptr);
            if (! ok) { return false; }
            if (written == 0) { return false; }
            done += written;
        }
        return true;
    }

    // Read a reply from the console input handle, with timeoutMs (similar semantics as POSIX version).
    // Termination when BEL ('\a') or ESC '\' sequence occurs.
    static std::expected<std::string, err_terminal> read_reply_from_console(HANDLE hIn, int timeoutMs = 500) noexcept {
        using clock          = std::chrono::steady_clock;
        auto        deadline = clock::now() + std::chrono::milliseconds(timeoutMs);
        std::string buf;
        buf.reserve(64);

        if (hIn == INVALID_HANDLE_VALUE || hIn == nullptr) { return std::unexpected(err_terminal::NoTerminal); }

        console_mode_guard guard(hIn);
        if (! guard.active) { return std::unexpected(err_terminal::IoError); }

        // We'll wait in short slices for readability and deadline checks.
        const DWORD sliceMs = 50;

        while (clock::now() < deadline) {
            // Wait for input availability
            DWORD remainMs = static_cast<DWORD>(
                std::chrono::duration_cast<std::chrono::milliseconds>(deadline - clock::now()).count());
            DWORD waitFor = (remainMs < sliceMs) ? remainMs : sliceMs;
            DWORD w       = WaitForSingleObject(hIn, waitFor);
            if (w == WAIT_FAILED) { return std::unexpected(err_terminal::IoError); }
            if (w == WAIT_TIMEOUT) {
                continue; // timeout slice
            }
            if (w == WAIT_OBJECT_0) {
                // There is something to read. Read one byte at a time to mimic POSIX behavior.
                char  ch;
                DWORD nread = 0;
                BOOL  ok    = ReadFile(hIn, &ch, 1, &nread, nullptr);
                if (! ok) {
                    DWORD err = GetLastError();
                    // If no data read yet but ReadFile returned false, consider IoError.
                    (void)err;
                    return std::unexpected(err_terminal::IoError);
                }
                if (nread == 0) {
                    // No more data (shouldn't normally happen), break to check deadline again.
                    continue;
                }
                buf.push_back(ch);
                // termination conditions: BEL or ESC '\' (ST)
                if (buf.back() == '\a') { break; }
                if (buf.size() >= 2 && buf[buf.size() - 2] == '\033' && buf.back() == '\\') { break; }
            }
        }

        if (buf.empty()) { return std::unexpected(err_terminal::Timeout); }
        return buf;
    }

    // Try to open CONOUT$ and CONIN$ (like /dev/tty). Prefer CONOUT/CONIN over std handles to avoid redirection issues.
    static std::pair<uniq_handle, uniq_handle> open_console_pair() noexcept {
        // open CONOUT$ for writing
        HANDLE hOut = CreateFileW(L"CONOUT$", GENERIC_WRITE | GENERIC_READ, FILE_SHARE_WRITE | FILE_SHARE_READ, nullptr,
                                  OPEN_EXISTING, 0, nullptr);
        uniq_handle out(hOut);

        // open CONIN$ for reading
        HANDLE hIn = CreateFileW(L"CONIN$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                                 OPEN_EXISTING, 0, nullptr);
        uniq_handle in(hIn);

        return {std::move(out), std::move(in)};
    }

    // parse_color_from_reply - copy of POSIX parser (keeps parity)
    static std::expected<inc_sRGB, err_terminal> parse_color_from_reply(std::string reply) noexcept {
        while (! reply.empty() && (reply.back() == '\r' || reply.back() == '\n')) { reply.pop_back(); }

        static std::regex rx_rgb(R"(rgb:([0-9A-Fa-f]{1,4})/([0-9A-Fa-f]{1,4})/([0-9A-Fa-f]{1,4}))");
        static std::regex rx_hash(R"(#([0-9A-Fa-f]{6}))");
        std::smatch       m;
        if (std::regex_search(reply, m, rx_rgb)) {
            auto to8 = [](std::string_view s) -> std::uint8_t {
                unsigned v = 0;
                std::from_chars(s.data(), s.data() + s.size(), v, 16);
                if (v > 0xFF) { v /= 257; }
                return static_cast<std::uint8_t>(v);
            };
            return inc_sRGB{to8(m[1].str()), to8(m[2].str()), to8(m[3].str())};
        }
        if (std::regex_search(reply, m, rx_hash)) {
            std::string hex   = m[1].str();
            auto        from2 = [&](int off) {
                unsigned v = 0;
                std::from_chars(hex.data() + off, hex.data() + off + 2, v, 16);
                return static_cast<std::uint8_t>(v);
            };
            return inc_sRGB{from2(0), from2(2), from2(4)};
        }
        return std::unexpected(err_terminal::ParseError);
    }

    // Make OSC query string (same as POSIX)
    static std::string make_osc_query(const std::string &body) { return std::string("\033]") + body + '\a'; }

    // send OSC and read reply using CONOUT/CONIN
    static std::expected<std::string, err_terminal> send_osc_and_read(const std::string &osc,
                                                                      int                timeoutMs = 500) noexcept {
        // Try to open CONOUT$/CONIN$ pair
        auto [out, in] = open_console_pair();
        if (! out.valid() || ! in.valid()) {
            // As a fallback, try standard handles if they are consoles
            HANDLE stdOut = GetStdHandle(STD_OUTPUT_HANDLE);
            HANDLE stdIn  = GetStdHandle(STD_INPUT_HANDLE);
            DWORD  mode;
            if (stdOut != INVALID_HANDLE_VALUE && stdOut != nullptr && GetConsoleMode(stdOut, &mode) &&
                stdIn != INVALID_HANDLE_VALUE && stdIn != nullptr && GetConsoleMode(stdIn, &mode)) {
                // use standard handles
                if (! write_all_handle(stdOut, osc.data(), osc.size())) {
                    return std::unexpected(err_terminal::IoError);
                }
                return read_reply_from_console(stdIn, timeoutMs);
            }
            return std::unexpected(err_terminal::NoTerminal);
        }

        // write OSC to out
        if (! write_all_handle(out.get(), osc.data(), osc.size())) { return std::unexpected(err_terminal::IoError); }

        // read reply from in
        return read_reply_from_console(in.get(), timeoutMs);
    }

    // query palette index (0..255) via OSC. For indexes <16, you may prefer GetConsoleScreenBufferInfoEx
    static Result queryPaletteIndex(int index) noexcept {
        if (! index256_valid(index)) { return std::unexpected(err_terminal::Unsupported); }

        // First, try ANSI OSC query (works on Windows Terminal / emulators supporting OSC replies)
        auto replyOrErr = send_osc_and_read(make_osc_query("4;" + std::to_string(index) + ";?"));
        if (! replyOrErr) { return std::unexpected(replyOrErr.error()); }
        auto rgbOrErr = parse_color_from_reply(*replyOrErr);
        if (! rgbOrErr) { return std::unexpected(rgbOrErr.error()); }
        return *rgbOrErr;
    }

    // query foreground via OSC 10
    static Result queryForeground() noexcept {
        auto replyOrErr = send_osc_and_read(make_osc_query("10;?"));
        if (! replyOrErr) { return std::unexpected(replyOrErr.error()); }
        auto rgbOrErr = parse_color_from_reply(*replyOrErr);
        if (! rgbOrErr) { return std::unexpected(rgbOrErr.error()); }
        return *rgbOrErr;
    }

    // query background via OSC 11
    static Result queryBackground() noexcept {
        auto replyOrErr = send_osc_and_read(make_osc_query("11;?"));
        if (! replyOrErr) { return std::unexpected(replyOrErr.error()); }
        auto rgbOrErr = parse_color_from_reply(*replyOrErr);
        if (! rgbOrErr) { return std::unexpected(rgbOrErr.error()); }
        return *rgbOrErr;
    }

    // Helper: read 16-entry color table using CONOUT$ / GetConsoleScreenBufferInfoEx if available.
    // Returns expected with color or IoError/NoTerminal if impossible.
    static std::expected<inc_sRGB, err_terminal> color_from_screenbuffer16(int index) noexcept {
        if (! index16_valid(index)) { return std::unexpected(err_terminal::Unsupported); }

        // Try CONOUT$ first (preferred)
        HANDLE hOut = CreateFileW(L"CONOUT$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                                  OPEN_EXISTING, 0, nullptr);
        if (hOut == INVALID_HANDLE_VALUE || hOut == nullptr) {
            // fallback to STD_OUTPUT_HANDLE if it is a console
            hOut = GetStdHandle(STD_OUTPUT_HANDLE);
            DWORD mode;
            if (hOut == INVALID_HANDLE_VALUE || hOut == nullptr || ! GetConsoleMode(hOut, &mode)) {
                return std::unexpected(err_terminal::NoTerminal);
            }
        }

        CONSOLE_SCREEN_BUFFER_INFOEX info;
        ZeroMemory(&info, sizeof(info));
        info.cbSize = sizeof(info);
        if (! GetConsoleScreenBufferInfoEx(hOut, &info)) {
            // If we opened CONOUT$ ourselves we should close it.
            if (hOut != GetStdHandle(STD_OUTPUT_HANDLE)) { CloseHandle(hOut); }
            return std::unexpected(err_terminal::IoError);
        }

        if (hOut != GetStdHandle(STD_OUTPUT_HANDLE)) { CloseHandle(hOut); }
        COLORREF cr = info.ColorTable[index];
        return inc_sRGB{static_cast<std::uint8_t>(GetRValue(cr)), static_cast<std::uint8_t>(GetGValue(cr)),
                        static_cast<std::uint8_t>(GetBValue(cr))};
    }


#else
    struct uniq_fd {
        int fd{-1};
        explicit uniq_fd(int f) : fd(f) {}
        ~uniq_fd() {
            if (fd >= 0) { ::close(fd); }
        }
        uniq_fd(const uniq_fd &)            = delete;
        uniq_fd &operator=(const uniq_fd &) = delete;
        uniq_fd(uniq_fd &&o) noexcept : fd(o.fd) { o.fd = -1; }
        uniq_fd &operator=(uniq_fd &&o) noexcept {
            if (fd >= 0) { ::close(fd); }
            fd   = o.fd;
            o.fd = -1;
            return *this;
        }
        bool valid() const noexcept { return fd >= 0; }
        int  get() const noexcept { return fd; }
    };

    struct termios_guard {
        int     fd{-1};
        termios old{};
        bool    active = false;
        explicit termios_guard(int f) : fd(f) {
            if (fd >= 0 && tcgetattr(fd, &old) == 0) {
                termios raw      = old;
                raw.c_lflag     &= ~(ICANON | ECHO);
                raw.c_cc[VMIN]   = 0;
                raw.c_cc[VTIME]  = 0;
                if (tcsetattr(fd, TCSANOW, &raw) == 0) { active = true; }
            }
        }
        ~termios_guard() {
            if (active) { tcsetattr(fd, TCSANOW, &old); }
        }
        termios_guard(const termios_guard &)            = delete;
        termios_guard &operator=(const termios_guard &) = delete;
    };

    static constexpr bool write_all(int fd, const char *data, size_t len) noexcept {
        size_t done = 0;
        while (done < len) {
            ssize_t n = ::write(fd, data + done, len - done);
            if (n < 0) {
                if (errno == EINTR) { continue; }
                return false;
            }
            done += size_t(n);
        }
        return true;
    }

    static constexpr std::expected<std::string, err_terminal> read_reply_from_tty(int ttyFd,
                                                                                  int timeoutMs = 500) noexcept {
        using clock          = std::chrono::steady_clock;
        auto        deadline = clock::now() + std::chrono::milliseconds(timeoutMs);
        std::string buf;
        buf.reserve(64);
        termios_guard guard(ttyFd);
        if (! guard.active) { return std::unexpected(err_terminal::IoError); }

        while (clock::now() < deadline) {
            fd_set rfds;
            FD_ZERO(&rfds);
            FD_SET(ttyFd, &rfds);
            auto           remain = std::chrono::duration_cast<std::chrono::microseconds>(deadline - clock::now());
            struct timeval tv{static_cast<time_t>(remain.count() / 1000000),
                              static_cast<suseconds_t>(remain.count() % 1000000)};
            int            r = select(ttyFd + 1, &rfds, nullptr, nullptr, &tv);
            if (r < 0) {
                if (errno == EINTR) { continue; }
                return std::unexpected(err_terminal::IoError);
            }
            if (r == 0) {
                continue; // timeout slice
            }
            if (FD_ISSET(ttyFd, &rfds)) {
                char    ch;
                ssize_t n = ::read(ttyFd, &ch, 1);
                if (n < 0) {
                    if (errno == EINTR) { continue; }
                    return std::unexpected(err_terminal::IoError);
                }
                if (n == 0) { break; }
                buf.push_back(ch);
                if (buf.back() == '\a') { break; }
                if (buf.size() >= 2 && buf[buf.size() - 2] == '\033' && buf.back() == '\\') { break; }
            }
        }
        if (buf.empty()) { return std::unexpected(err_terminal::Timeout); }
        return buf;
    }

    static constexpr std::expected<std::string, err_terminal> send_osc_and_read(const std::string &osc,
                                                                                int timeoutMs = 500) noexcept {
        if (not isatty(STDOUT_FILENO) && not isatty(STDIN_FILENO) && not isatty(STDERR_FILENO)) {
            return std::unexpected(err_terminal::NoTerminal);
        }
        uniq_fd tty(::open("/dev/tty", O_RDWR | O_NOCTTY));
        if (! tty.valid()) { return std::unexpected(err_terminal::NoTerminal); }
        if (! write_all(tty.get(), osc.data(), osc.size())) { return std::unexpected(err_terminal::IoError); }
        return read_reply_from_tty(tty.get(), timeoutMs);
    }

    static constexpr std::expected<inc_sRGB, err_terminal> parse_color_from_reply(std::string reply) noexcept {
        while (! reply.empty() && (reply.back() == '\r' || reply.back() == '\n')) { reply.pop_back(); }

        static std::regex rx_rgb(R"(rgb:([0-9A-Fa-f]{1,4})/([0-9A-Fa-f]{1,4})/([0-9A-Fa-f]{1,4}))");
        static std::regex rx_hash(R"(#([0-9A-Fa-f]{6}))");
        std::smatch       m;
        if (std::regex_search(reply, m, rx_rgb)) {
            auto to8 = [](std::string_view s) -> std::uint8_t {
                unsigned v = 0;
                std::from_chars(s.data(), s.data() + s.size(), v, 16);
                if (v > 0xFF) { v /= 257; }
                return static_cast<std::uint8_t>(v);
            };
            return inc_sRGB{to8(m[1].str()), to8(m[2].str()), to8(m[3].str())};
        }
        if (std::regex_search(reply, m, rx_hash)) {
            std::string hex   = m[1].str();
            auto        from2 = [&](int off) {
                unsigned v = 0;
                std::from_chars(hex.data() + off, hex.data() + off + 2, v, 16);
                return static_cast<std::uint8_t>(v);
            };
            return inc_sRGB{from2(0), from2(2), from2(4)};
        }
        return std::unexpected(err_terminal::ParseError);
    }

    static constexpr std::string make_osc_query(const std::string &body) { return "\033]" + body + '\a'; }

    static constexpr Result queryPaletteIndex(int index) noexcept {
        if (! index256_valid(index)) { return std::unexpected(err_terminal::Unsupported); }
        auto replyOrErr = send_osc_and_read(make_osc_query("4;" + std::to_string(index) + ";?"));
        if (! replyOrErr) { return std::unexpected(replyOrErr.error()); }
        auto rgbOrErr = parse_color_from_reply(*replyOrErr);
        if (! rgbOrErr) { return std::unexpected(rgbOrErr.error()); }
        return *rgbOrErr;
    }

    static constexpr Result queryForeground() noexcept {
        auto replyOrErr = send_osc_and_read(make_osc_query("10;?"));
        if (! replyOrErr) { return std::unexpected(replyOrErr.error()); }
        auto rgbOrErr = parse_color_from_reply(*replyOrErr);
        if (! rgbOrErr) { return std::unexpected(rgbOrErr.error()); }
        return *rgbOrErr;
    }

    static constexpr Result queryBackground() noexcept {
        auto replyOrErr = send_osc_and_read(make_osc_query("11;?"));
        if (! replyOrErr) { return std::unexpected(replyOrErr.error()); }
        auto rgbOrErr = parse_color_from_reply(*replyOrErr);
        if (! rgbOrErr) { return std::unexpected(rgbOrErr.error()); }
        return *rgbOrErr;
    }
#endif
};

} // namespace incom::standard::console
