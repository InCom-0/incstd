#pragma once
#include <array>
#include <charconv>
#include <chrono>
#include <cstdint>
#include <expected>
#include <regex>
#include <string>
#include <string_view>

#ifdef _MSC_VER
#define NOMINMAX
#endif

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
    [[nodiscard]] static constexpr result_t get_paletteIdx(int index) {
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
    [[nodiscard]] static constexpr result_t get_foreground() {
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
    [[nodiscard]] static constexpr result_t get_background() {
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
        if (! index256_valid(index)) { return inc_sRGB{255, 255, 255}; }
        return color_schemes::defaultScheme256.palette[index];
    }

private:
    // ────────────── INTERNAL ──────────────

    [[nodiscard]] static constexpr bool index16_valid(int idx) noexcept { return idx >= 0 && idx <= 15; }
    [[nodiscard]] static constexpr bool index256_valid(int idx) noexcept { return idx >= 0 && idx <= 255; }

#ifdef _WIN32
    // So far nothing here
    // In the future might have some implementation once Windows terminal reports actually used colors with OSC 4

    static constexpr result_t queryPaletteIndex(int index) noexcept {
        return std::unexpected(err_terminal::Unsupported);
    }

    static constexpr result_t queryForeground() noexcept { return std::unexpected(err_terminal::Unsupported); }

    static constexpr result_t queryBackground() noexcept { return std::unexpected(err_terminal::Unsupported); }

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

    static constexpr result_t queryPaletteIndex(int index) noexcept {
        if (! index256_valid(index)) { return std::unexpected(err_terminal::Unsupported); }
        auto replyOrErr = send_osc_and_read(make_osc_query("4;" + std::to_string(index) + ";?"));
        if (! replyOrErr) { return std::unexpected(replyOrErr.error()); }
        auto rgbOrErr = parse_color_from_reply(*replyOrErr);
        if (! rgbOrErr) { return std::unexpected(rgbOrErr.error()); }
        return *rgbOrErr;
    }

    static constexpr result_t queryForeground() noexcept {
        auto replyOrErr = send_osc_and_read(make_osc_query("10;?"));
        if (! replyOrErr) { return std::unexpected(replyOrErr.error()); }
        auto rgbOrErr = parse_color_from_reply(*replyOrErr);
        if (! rgbOrErr) { return std::unexpected(rgbOrErr.error()); }
        return *rgbOrErr;
    }

    static constexpr result_t queryBackground() noexcept {
        auto replyOrErr = send_osc_and_read(make_osc_query("11;?"));
        if (! replyOrErr) { return std::unexpected(replyOrErr.error()); }
        auto rgbOrErr = parse_color_from_reply(*replyOrErr);
        if (! rgbOrErr) { return std::unexpected(rgbOrErr.error()); }
        return *rgbOrErr;
    }
#endif
};

} // namespace incom::standard::console
