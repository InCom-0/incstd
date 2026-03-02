#pragma once
#include <algorithm>
#include <array>
#include <charconv>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <expected>
#include <regex>
#include <string>
#include <string_view>
#include <type_traits>


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
    using result_t = std::expected<inc_sRGB, err_terminal>;

    // query palette index 0..255 (returns expected)
    [[nodiscard]] static constexpr result_t get_paletteIdx(int index) { return queryPaletteIndex(index); }

    // convenience: always returns something (default palette on failure)
    [[nodiscard]] static constexpr inc_sRGB get_paletteIdx_fb(int index) noexcept {
        auto res = queryPaletteIndex(index);
        return res ? *res : get_defaultColor(index);
    }

    // foreground
    [[nodiscard]] static constexpr result_t get_foreground() { return queryForeground(); }

    [[nodiscard]] static constexpr inc_sRGB get_foreground_fb() noexcept {
        auto res = queryForeground();
        return res ? *res : get_defaultColor(7);
    }

    // background
    [[nodiscard]] static constexpr result_t get_background() { return queryBackground(); }

    [[nodiscard]] static constexpr inc_sRGB get_background_fb() noexcept {
        auto res = queryBackground();
        return res ? *res : get_defaultColor(0);
    }

    // cursor
    [[nodiscard]] static constexpr result_t get_cursorCol() {
#ifdef _WIN32
        return queryCursorCol();
#else
        return queryBackground();
#endif
    }

    [[nodiscard]] static constexpr inc_sRGB get_cursorCol_fb() noexcept {
#ifdef _WIN32
        auto res = queryCursorCol();
        return res ? *res : get_defaultColor(static_cast<int>(ANSI_Color16::Bright_White));
#else
        auto res = queryBackground();
        return res ? *res : get_defaultColor(0);
#endif
    }

    // get all 16 colors at once
    [[nodiscard]] static constexpr std::expected<palette16, err_terminal> get_palette16() noexcept {
        palette16 colors{};
        for (int i = 0; i < std::tuple_size_v<decltype(colors)>; ++i) {
            auto res = queryPaletteIndex(i);
            if (res.has_value()) { colors[i] = res.value(); }
            else { return std::unexpected(res.error()); }
        }
        return colors;
    }

    // get all 16 colors at once with fallback
    [[nodiscard]] static constexpr std::expected<palette16, err_terminal> get_palette16_fb() noexcept {
        palette16 colors{};
        for (int i = 0; i < std::tuple_size_v<decltype(colors)>; ++i) {
            auto res = queryPaletteIndex(i);
            if (res.has_value()) { colors[i] = res.value(); }
            else { colors[i] = get_defaultColor(i); }
        }
        return colors;
    }

    // get all 256 colors at once
    [[nodiscard]] static constexpr std::expected<palette256, err_terminal> get_palette256() noexcept {
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
            auto res = queryPaletteIndex(i);
            if (res.has_value()) { colors[i] = res.value(); }
            else { colors[i] = get_defaultColor(i); }
        }
        return colors;
    }

    // Get color from the 'default' palette
    [[nodiscard]] static constexpr inc_sRGB get_defaultColor(int index) noexcept {
        if (! index256_valid(index)) { return inc_sRGB{255, 255, 255}; }
        return color_schemes::defaultScheme256.palette[index];
    }

private:
    // ────────────── INTERNAL ──────────────

    [[nodiscard]] static constexpr bool index16_valid(int idx) noexcept { return idx >= 0 && idx <= 15; }
    [[nodiscard]] static constexpr bool index256_valid(int idx) noexcept { return idx >= 0 && idx <= 255; }

#ifdef _WIN32
    struct handle_guard {
        HANDLE h{INVALID_HANDLE_VALUE};
        explicit handle_guard(HANDLE handle) : h(handle) {}
        ~handle_guard() {
            if (h != INVALID_HANDLE_VALUE) { ::CloseHandle(h); }
        }
        handle_guard(const handle_guard &)            = delete;
        handle_guard &operator=(const handle_guard &) = delete;
        handle_guard(handle_guard &&o) noexcept : h(o.h) { o.h = INVALID_HANDLE_VALUE; }
        handle_guard &operator=(handle_guard &&o) noexcept {
            if (h != INVALID_HANDLE_VALUE) { ::CloseHandle(h); }
            h   = o.h;
            o.h = INVALID_HANDLE_VALUE;
            return *this;
        }
        bool   valid() const noexcept { return h != INVALID_HANDLE_VALUE; }
        HANDLE get() const noexcept { return h; }
    };

    struct console_mode_guard {
        HANDLE h{INVALID_HANDLE_VALUE};
        DWORD  oldMode{};
        bool   active = false;
        console_mode_guard(HANDLE handle, DWORD setMask, DWORD clearMask) : h(handle) {
            if (h == INVALID_HANDLE_VALUE) { return; }
            if (::GetConsoleMode(h, &oldMode)) {
                DWORD mode = (oldMode | setMask) & ~clearMask;
                if (::SetConsoleMode(h, mode)) { active = true; }
            }
        }
        ~console_mode_guard() {
            if (active) { ::SetConsoleMode(h, oldMode); }
        }
        console_mode_guard(const console_mode_guard &)            = delete;
        console_mode_guard &operator=(const console_mode_guard &) = delete;
    };

    static constexpr bool write_all(HANDLE h, const char *data, size_t len) noexcept {
        if (std::is_constant_evaluated()) { return false; }
        size_t done = 0;
        while (done < len) {
            DWORD wrote = 0;
            if (! ::WriteConsoleA(h, data + done, static_cast<DWORD>(len - done), &wrote, nullptr)) {
                if (! ::WriteFile(h, data + done, static_cast<DWORD>(len - done), &wrote, nullptr)) { return false; }
            }
            if (wrote == 0) { return false; }
            done += static_cast<size_t>(wrote);
        }
        return true;
    }

    static constexpr std::expected<std::string, err_terminal> read_reply_from_console(HANDLE hIn,
                                                                                      int    timeoutMs = 500) noexcept {
        if (std::is_constant_evaluated()) { return std::unexpected(err_terminal::Unsupported); }
        using clock          = std::chrono::steady_clock;
        auto        deadline = clock::now() + std::chrono::milliseconds(timeoutMs);
        std::string buf;
        buf.reserve(64);

        const DWORD inType      = ::GetFileType(hIn);
        const bool  isConsoleIn = (inType == FILE_TYPE_CHAR);

        while (clock::now() < deadline) {
            auto  remain     = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - clock::now());
            DWORD sliceMs    = static_cast<DWORD>(std::min<int64_t>(50, remain.count()));
            DWORD waitStatus = ::WaitForSingleObject(hIn, sliceMs);
            if (waitStatus == WAIT_TIMEOUT) { continue; }
            if (waitStatus != WAIT_OBJECT_0) { return std::unexpected(err_terminal::IoError); }

            char  ch   = 0;
            DWORD read = 0;
            if (isConsoleIn) {
                if (! ::ReadConsoleA(hIn, &ch, 1, &read, nullptr)) { return std::unexpected(err_terminal::IoError); }
            }
            else {
                if (! ::ReadFile(hIn, &ch, 1, &read, nullptr)) { return std::unexpected(err_terminal::IoError); }
            }
            if (read == 0) { break; }
            buf.push_back(ch);
            if (buf.back() == '\a') { break; }
            if (buf.size() >= 2 && buf[buf.size() - 2] == '\033' && buf.back() == '\\') { break; }
        }
        if (buf.empty()) { return std::unexpected(err_terminal::Timeout); }
        return buf;
    }

    static constexpr std::expected<std::string, err_terminal> send_osc_and_read(const std::string &osc,
                                                                                int timeoutMs = 500) noexcept {
        if (std::is_constant_evaluated()) { return std::unexpected(err_terminal::Unsupported); }

        handle_guard hOut(::CreateFileA("CONOUT$", GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                        nullptr, OPEN_EXISTING, 0, nullptr));
        handle_guard hIn(::CreateFileA("CONIN$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                       nullptr, OPEN_EXISTING, 0, nullptr));
        if (! hOut.valid() || ! hIn.valid()) { return std::unexpected(err_terminal::NoTerminal); }

        const DWORD outType = ::GetFileType(hOut.get());
        const DWORD inType  = ::GetFileType(hIn.get());

        console_mode_guard outMode(hOut.get(), ENABLE_VIRTUAL_TERMINAL_PROCESSING, 0);
        if (outType == FILE_TYPE_CHAR && ! outMode.active) { return std::unexpected(err_terminal::IoError); }

        console_mode_guard inMode(hIn.get(), ENABLE_VIRTUAL_TERMINAL_INPUT, ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT);
        if (inType == FILE_TYPE_CHAR && ! inMode.active) { return std::unexpected(err_terminal::IoError); }

        if (! write_all(hOut.get(), osc.data(), osc.size())) { return std::unexpected(err_terminal::IoError); }
        auto reply = read_reply_from_console(hIn.get(), timeoutMs);

        if (! reply.has_value() && reply.error() == err_terminal::Timeout) {
            const char *termProgram = std::getenv("TERM_PROGRAM");
            if (termProgram != nullptr && std::string_view(termProgram) == "vscode") {
                return std::unexpected(err_terminal::Unsupported);
            }
        }

        return reply;
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

#endif

    // TODO: This is temporarily kept non-constexpr because MSVC is not up to date on constexpr implementation of some
    // stuff this depends on (std::regex_search)
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

    static constexpr std::string make_osc_query(const std::string &body) { return "\033]" + body + '\a'; }

    static constexpr result_t queryPaletteIndex(int index) noexcept {
        if (! index256_valid(index)) { return std::unexpected(err_terminal::Unsupported); }
        return send_osc_and_read(make_osc_query("4;" + std::to_string(index) + ";?")).and_then(parse_color_from_reply);
    }

    static constexpr result_t queryForeground() noexcept {
        return send_osc_and_read(make_osc_query("10;?")).and_then(parse_color_from_reply);
    }

    static constexpr result_t queryBackground() noexcept {
        return send_osc_and_read(make_osc_query("11;?")).and_then(parse_color_from_reply);
    }

    static constexpr result_t queryCursorCol() noexcept {
        return send_osc_and_read(make_osc_query("12;?")).and_then(parse_color_from_reply);
    }
};

} // namespace incom::standard::console
