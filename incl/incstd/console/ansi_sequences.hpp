#pragma once

#include <cstddef>
#include <string>
#include <string_view>

#include <incstd/color/color_common.hpp>


namespace incom::standard::console {
namespace ANSI {
using namespace incom::standard::color;
using namespace std::literals;

enum class SGR_map : std::size_t {
    Reset = 0,

    Bold       = 1,
    Faint      = 2,
    Italic     = 3,
    Underline  = 4,
    SlowBlink  = 5,
    RapidBlink = 6,
    Inverse    = 7,
    Conceal    = 8,
    CrossedOut = 9,

    Fraktur         = 20,
    DoubleUnderline = 21, // sometimes "BoldOff" depending on terminal
    NormalIntensity = 22,
    ItalicOff       = 23,
    UnderlineOff    = 24,
    BlinkOff        = 25,
    InverseOff      = 27,
    ConcealOff      = 28,
    CrossedOutOff   = 29,

    FG_Black   = 30,
    FG_Red     = 31,
    FG_Green   = 32,
    FG_Yellow  = 33,
    FG_Blue    = 34,
    FG_Magenta = 35,
    FG_Cyan    = 36,
    FG_White   = 37,
    FG_Default = 39,

    BG_Black   = 40,
    BG_Red     = 41,
    BG_Green   = 42,
    BG_Yellow  = 43,
    BG_Blue    = 44,
    BG_Magenta = 45,
    BG_Cyan    = 46,
    BG_White   = 47,
    BG_Default = 49,

    Framed      = 51,
    Encircled   = 52,
    Overlined   = 53,
    FrameOff    = 54, // frame/encircle off
    OverlineOff = 55,

    FG_BrightBlack   = 90,
    FG_BrightRed     = 91,
    FG_BrightGreen   = 92,
    FG_BrightYellow  = 93,
    FG_BrightBlue    = 94,
    FG_BrightMagenta = 95,
    FG_BrightCyan    = 96,
    FG_BrightWhite   = 97,

    BG_BrightBlack   = 100,
    BG_BrightRed     = 101,
    BG_BrightGreen   = 102,
    BG_BrightYellow  = 103,
    BG_BrightBlue    = 104,
    BG_BrightMagenta = 105,
    BG_BrightCyan    = 106,
    BG_BrightWhite   = 107,
};

inline constexpr std::array<std::string_view, 108> SGR_direct = {
    /* 0 */ "\x1b[m"sv,  // reset
    /* 1 */ "\x1b[1m"sv, // bold on
    /* 2 */ "\x1b[2m"sv, // faint
    /* 3 */ "\x1b[3m"sv, // italic
    /* 4 */ "\x1b[4m"sv, // underline
    /* 5 */ "\x1b[5m"sv, // slow blink
    /* 6 */ "\x1b[6m"sv, // rapid blink
    /* 7 */ "\x1b[7m"sv, // inverse
    /* 8 */ "\x1b[8m"sv, // conceal
    /* 9 */ "\x1b[9m"sv, // crossed-out

    /* 10–19 */ ""sv, ""sv, ""sv, ""sv, ""sv, ""sv, ""sv, ""sv, ""sv, ""sv,

    /* 20 */ "\x1b[20m"sv, // fraktur (rare)
    /* 21 */ "\x1b[21m"sv, // double underline or bold off (varies)
    /* 22 */ "\x1b[22m"sv, // normal intensity
    /* 23 */ "\x1b[23m"sv, // italic off
    /* 24 */ "\x1b[24m"sv, // underline off
    /* 25 */ "\x1b[25m"sv, // blink off
    /* 26 */ ""sv,
    /* 27 */ "\x1b[27m"sv, // inverse off
    /* 28 */ "\x1b[28m"sv, // conceal off
    /* 29 */ "\x1b[29m"sv, // crossed-out off

    /* 30–37: set foreground color (standard) */
    "\x1b[30m"sv, "\x1b[31m"sv, "\x1b[32m"sv, "\x1b[33m"sv, "\x1b[34m"sv, "\x1b[35m"sv, "\x1b[36m"sv, "\x1b[37m"sv,
    /* 38 */ ""sv,         // (needs parameters: extended fg)
    /* 39 */ "\x1b[39m"sv, // default fg

    /* 40–47: set background color (standard) */
    "\x1b[40m"sv, "\x1b[41m"sv, "\x1b[42m"sv, "\x1b[43m"sv, "\x1b[44m"sv, "\x1b[45m"sv, "\x1b[46m"sv, "\x1b[47m"sv,
    /* 48 */ ""sv,         // (needs parameters: extended bg)
    /* 49 */ "\x1b[49m"sv, // default bg

    /* 50 */ ""sv,
    /* 51 */ "\x1b[51m"sv, // framed
    /* 52 */ "\x1b[52m"sv, // encircled
    /* 53 */ "\x1b[53m"sv, // overlined
    /* 54 */ "\x1b[54m"sv, // frame/encircle off
    /* 55 */ "\x1b[55m"sv, // overline off

    /* 56–59 */ ""sv, ""sv, ""sv, ""sv,

    /* 60–65 */ ""sv, ""sv, ""sv, ""sv, ""sv, ""sv,

    /* 66–74 */ ""sv, ""sv, ""sv, ""sv, ""sv, ""sv, ""sv, ""sv, ""sv,

    /* 75 */ ""sv,
    /* 76–89 */ ""sv, ""sv, ""sv, ""sv, ""sv, ""sv, ""sv, ""sv, ""sv, ""sv, ""sv, ""sv, ""sv, ""sv,

    /* 90–97: bright fg colors */
    "\x1b[90m"sv, "\x1b[91m"sv, "\x1b[92m"sv, "\x1b[93m"sv, "\x1b[94m"sv, "\x1b[95m"sv, "\x1b[96m"sv, "\x1b[97m"sv,

    /* 98 */ ""sv,
    /* 99 */ ""sv,

    /* 100–107: bright bg colors */
    "\x1b[100m"sv, "\x1b[101m"sv, "\x1b[102m"sv, "\x1b[103m"sv, "\x1b[104m"sv, "\x1b[105m"sv, "\x1b[106m"sv,
    "\x1b[107m"sv};

constexpr SGR_map ANSI_col16_to_SGRmap_fg(ANSI_Color16 col) {
    return SGR_map{static_cast<int>(col) > 7 ? static_cast<size_t>(col) + 82uz : static_cast<size_t>(col) + 30uz};
}
constexpr SGR_map ANSI_col16_to_SGRmap_bg(ANSI_Color16 col) {
    return SGR_map{static_cast<int>(col) > 7 ? static_cast<size_t>(col) + 92uz : static_cast<size_t>(col) + 40uz};
}

constexpr std::string_view const &get_fromSGR_direct(SGR_map code) {
    return SGR_direct[static_cast<int>(code)];
}

constexpr std::string_view get_fg(ANSI_Color16 const col) {
    return SGR_direct[static_cast<int>(col) > 7 ? static_cast<int>(col) + 82 : static_cast<int>(col) + 30];
}
constexpr std::string get_fg(std::uint8_t const color_256) {
    return std::string("\x1b[38;5;").append(std::to_string(static_cast<int>(color_256))).append("m");
}
constexpr std::string get_fg(std::uint8_t const r, std::uint8_t const g, std::uint8_t const b) {
    std::string res("\x1b[38;2;");
    res.append(std::to_string(r)).push_back(';');
    res.append(std::to_string(g)).push_back(';');
    res.append(std::to_string(b)).push_back('m');
    return res;
}
constexpr std::string get_fg(inc_sRGB const color) {
    return get_fg(color.r, color.g, color.b);
}


constexpr std::string_view get_bg(ANSI_Color16 const col) {
    return SGR_direct[static_cast<int>(col) > 7 ? static_cast<int>(col) + 92 : static_cast<int>(col) + 40];
}
constexpr std::string get_bg(std::uint8_t const color_256) {
    return std::string("\x1b[48;5;").append(std::to_string(static_cast<int>(color_256))).append("m");
}
constexpr std::string get_bg(std::uint8_t const r, std::uint8_t const g, std::uint8_t const b) {
    std::string res("\x1b[48;2;");
    res.append(std::to_string(r)).push_back(';');
    res.append(std::to_string(g)).push_back(';');
    res.append(std::to_string(b)).push_back('m');
    return res;
}
constexpr std::string get_bg(inc_sRGB const color) {
    return get_bg(color.r, color.g, color.b);
}

class SGR_builder {
public:
    constexpr std::string get() && { return std::move(_res); }
    constexpr std::string get() const && { return _res; }
    constexpr std::string get() & { return _res; }
    constexpr std::string get() const & { return _res; }

    constexpr const std::string &get_asRef() const & { return _res; }

    // #######################################
    // ### BUILDER METHODS      ##############
    // #######################################

    constexpr SGR_builder &add_SGR_direct(SGR_map code) & {
        _res.append(SGR_direct[static_cast<int>(code)]);
        return *this;
    }
    constexpr SGR_builder &&add_SGR_direct(SGR_map code) && {
        _res.append(SGR_direct[static_cast<int>(code)]);
        return std::move(*this);
    }

    constexpr SGR_builder &add_string(std::string_view sv) & {
        _res.append(sv);
        return *this;
    }
    constexpr SGR_builder &&add_string(std::string_view sv) && {
        _res.append(sv);
        return std::move(*this);
    }

    constexpr SGR_builder &reset_all() & {
        _res.append("\x1b[m"sv);
        return *this;
    }
    constexpr SGR_builder &&reset_all() && {
        _res.append("\x1b[m"sv);
        return std::move(*this);
    }

    // ---- foreground colors ----
    constexpr SGR_builder &color_fg(ANSI_Color16 const col) & {
        _res.append(get_fg(col));
        return *this;
    }
    constexpr SGR_builder &&color_fg(ANSI_Color16 const col) && {
        _res.append(get_fg(col));
        return std::move(*this);
    }

    constexpr SGR_builder &color_fg(std::uint8_t const color_256) & {
        _res.append("\x1b[38;5;").append(std::to_string(static_cast<int>(color_256))).append("m");
        return *this;
    }
    constexpr SGR_builder &&color_fg(std::uint8_t const color_256) && {
        _res.append("\x1b[38;5;").append(std::to_string(static_cast<int>(color_256))).append("m");
        return std::move(*this);
    }

    constexpr SGR_builder &color_fg(std::uint8_t const r, std::uint8_t const g, std::uint8_t const b) & {
        _res.append("\x1b[38;2;");
        _res.append(std::to_string(r)).push_back(';');
        _res.append(std::to_string(g)).push_back(';');
        _res.append(std::to_string(b)).push_back('m');
        return *this;
    }
    constexpr SGR_builder &&color_fg(std::uint8_t const r, std::uint8_t const g, std::uint8_t const b) && {
        _res.append("\x1b[38;2;");
        _res.append(std::to_string(r)).push_back(';');
        _res.append(std::to_string(g)).push_back(';');
        _res.append(std::to_string(b)).push_back('m');
        return std::move(*this);
    }

    constexpr SGR_builder  &color_fg(inc_sRGB const color)  &{ return this->color_fg(color.r, color.g, color.b); }
    constexpr SGR_builder &&color_fg(inc_sRGB const color) && {
        return std::move(*this).color_fg(color.r, color.g, color.b);
    }

    // ---- background colors ----
    constexpr SGR_builder &color_bg(ANSI_Color16 const col) & {
        _res.append(get_bg(col));
        return *this;
    }
    constexpr SGR_builder &&color_bg(ANSI_Color16 const col) && {
        _res.append(get_bg(col));
        return std::move(*this);
    }

    constexpr SGR_builder &color_bg(std::uint8_t const color_256) & {
        _res.append("\x1b[48;5;").append(std::to_string(static_cast<int>(color_256))).append("m");
        return *this;
    }
    constexpr SGR_builder &&color_bg(std::uint8_t const color_256) && {
        _res.append("\x1b[48;5;").append(std::to_string(static_cast<int>(color_256))).append("m");
        return std::move(*this);
    }

    constexpr SGR_builder &color_bg(std::uint8_t const r, std::uint8_t const g, std::uint8_t const b) & {
        _res.append("\x1b[48;2;");
        _res.append(std::to_string(r)).push_back(';');
        _res.append(std::to_string(g)).push_back(';');
        _res.append(std::to_string(b)).push_back('m');
        return *this;
    }
    constexpr SGR_builder &&color_bg(std::uint8_t const r, std::uint8_t const g, std::uint8_t const b) && {
        _res.append("\x1b[48;2;");
        _res.append(std::to_string(r)).push_back(';');
        _res.append(std::to_string(g)).push_back(';');
        _res.append(std::to_string(b)).push_back('m');
        return std::move(*this);
    }

    constexpr SGR_builder  &color_bg(inc_sRGB const color)  &{ return this->color_bg(color.r, color.g, color.b); }
    constexpr SGR_builder &&color_bg(inc_sRGB const color) && {
        return std::move(*this).color_bg(color.r, color.g, color.b);
    }

    // ---- styles ----
    constexpr SGR_builder &color_fg_default() & {
        _res.append(SGR_direct[39]);
        return *this;
    }
    constexpr SGR_builder &&color_fg_default() && {
        _res.append(SGR_direct[39]);
        return std::move(*this);
    }

    constexpr SGR_builder &color_bg_default() & {
        _res.append(SGR_direct[49]);
        return *this;
    }
    constexpr SGR_builder &&color_bg_default() && {
        _res.append(SGR_direct[49]);
        return std::move(*this);
    }

    constexpr SGR_builder &bold() & {
        _res.append(SGR_direct[1]);
        return *this;
    }
    constexpr SGR_builder &&bold() && {
        _res.append(SGR_direct[1]);
        return std::move(*this);
    }

    constexpr SGR_builder &faint() & {
        _res.append(SGR_direct[2]);
        return *this;
    }
    constexpr SGR_builder &&faint() && {
        _res.append(SGR_direct[2]);
        return std::move(*this);
    }

    constexpr SGR_builder &italic() & {
        _res.append(SGR_direct[3]);
        return *this;
    }
    constexpr SGR_builder &&italic() && {
        _res.append(SGR_direct[3]);
        return std::move(*this);
    }

    constexpr SGR_builder &underline() & {
        _res.append(SGR_direct[4]);
        return *this;
    }
    constexpr SGR_builder &&underline() && {
        _res.append(SGR_direct[4]);
        return std::move(*this);
    }

    constexpr SGR_builder &underlineDouble() & {
        _res.append(SGR_direct[21]);
        return *this;
    }
    constexpr SGR_builder &&underlineDouble() && {
        _res.append(SGR_direct[21]);
        return std::move(*this);
    }

    constexpr SGR_builder &overline() & {
        _res.append(SGR_direct[53]);
        return *this;
    }
    constexpr SGR_builder &&overline() && {
        _res.append(SGR_direct[53]);
        return std::move(*this);
    }

    constexpr SGR_builder &strike() & {
        _res.append(SGR_direct[9]);
        return *this;
    }
    constexpr SGR_builder &&strike() && {
        _res.append(SGR_direct[9]);
        return std::move(*this);
    }

    constexpr SGR_builder &conceal() & {
        _res.append(SGR_direct[8]);
        return *this;
    }
    constexpr SGR_builder &&conceal() && {
        _res.append(SGR_direct[8]);
        return std::move(*this);
    }

    constexpr SGR_builder &normalIntensity() & {
        _res.append(SGR_direct[22]);
        return *this;
    }
    constexpr SGR_builder &&normalIntensity() && {
        _res.append(SGR_direct[22]);
        return std::move(*this);
    }

    constexpr SGR_builder &notItalic() & {
        _res.append(SGR_direct[23]);
        return *this;
    }
    constexpr SGR_builder &&notItalic() && {
        _res.append(SGR_direct[23]);
        return std::move(*this);
    }

    constexpr SGR_builder &notUnderline() & {
        _res.append(SGR_direct[24]);
        return *this;
    }
    constexpr SGR_builder &&notUnderline() && {
        _res.append(SGR_direct[24]);
        return std::move(*this);
    }

    constexpr SGR_builder &notOverline() & {
        _res.append(SGR_direct[55]);
        return *this;
    }
    constexpr SGR_builder &&notOverline() && {
        _res.append(SGR_direct[55]);
        return std::move(*this);
    }

    constexpr SGR_builder &notStrike() & {
        _res.append(SGR_direct[29]);
        return *this;
    }
    constexpr SGR_builder &&notStrike() && {
        _res.append(SGR_direct[29]);
        return std::move(*this);
    }

    constexpr SGR_builder &reveal() & {
        _res.append(SGR_direct[28]);
        return *this;
    }
    constexpr SGR_builder &&reveal() && {
        _res.append(SGR_direct[28]);
        return std::move(*this);
    }

    constexpr SGR_builder &framed() & {
        _res.append(SGR_direct[51]);
        return *this;
    }
    constexpr SGR_builder &&framed() && {
        _res.append(SGR_direct[51]);
        return std::move(*this);
    }

    constexpr SGR_builder &encircled() & {
        _res.append(SGR_direct[52]);
        return *this;
    }
    constexpr SGR_builder &&encircled() && {
        _res.append(SGR_direct[52]);
        return std::move(*this);
    }

    constexpr SGR_builder &notFrameEncircled() & {
        _res.append(SGR_direct[54]);
        return *this;
    }
    constexpr SGR_builder &&notFrameEncircled() && {
        _res.append(SGR_direct[54]);
        return std::move(*this);
    }

    constexpr SGR_builder &underlineColor() & {
        _res.append(SGR_direct[59]);
        return *this;
    }
    constexpr SGR_builder &&underlineColor() && {
        _res.append(SGR_direct[59]);
        return std::move(*this);
    }

    constexpr SGR_builder &underlineColor_default() & {
        _res.append(SGR_direct[59]);
        return *this;
    }
    constexpr SGR_builder &&underlineColor_default() && {
        _res.append(SGR_direct[59]);
        return std::move(*this);
    }

    constexpr SGR_builder &superscript() & {
        _res.append(SGR_direct[73]);
        return *this;
    }
    constexpr SGR_builder &&superscript() && {
        _res.append(SGR_direct[73]);
        return std::move(*this);
    }

    constexpr SGR_builder &subscript() & {
        _res.append(SGR_direct[74]);
        return *this;
    }
    constexpr SGR_builder &&subscript() && {
        _res.append(SGR_direct[74]);
        return std::move(*this);
    }

    constexpr SGR_builder &notSuperSubScript() & {
        _res.append(SGR_direct[75]);
        return *this;
    }
    constexpr SGR_builder &&notSuperSubScript() && {
        _res.append(SGR_direct[75]);
        return std::move(*this);
    }

    constexpr SGR_builder &colors_inverted() & {
        _res.append(SGR_direct[7]);
        return *this;
    }
    constexpr SGR_builder &&colors_inverted() && {
        _res.append(SGR_direct[7]);
        return std::move(*this);
    }

    constexpr SGR_builder &colors_reverted() & {
        _res.append(SGR_direct[27]);
        return *this;
    }
    constexpr SGR_builder &&colors_reverted() && {
        _res.append(SGR_direct[27]);
        return std::move(*this);
    }

    constexpr SGR_builder &blinkSlow() & {
        _res.append(SGR_direct[5]);
        return *this;
    }
    constexpr SGR_builder &&blinkSlow() && {
        _res.append(SGR_direct[5]);
        return std::move(*this);
    }

    constexpr SGR_builder &blinkFast() & {
        _res.append(SGR_direct[6]);
        return *this;
    }
    constexpr SGR_builder &&blinkFast() && {
        _res.append(SGR_direct[6]);
        return std::move(*this);
    }

    constexpr SGR_builder &notBlink() & {
        _res.append(SGR_direct[25]);
        return *this;
    }
    constexpr SGR_builder &&notBlink() && {
        _res.append(SGR_direct[25]);
        return std::move(*this);
    }

    // Support for ostream usage through a non-member friend function
    friend std::ostream &operator<<(std::ostream &os, const SGR_builder &obj) { return os << obj.get_asRef(); }
    friend std::ostream &operator<<(std::ostream &os, SGR_builder &&obj) { return os << std::move(obj).get(); }

private:
    std::string _res;

    constexpr int _toSGR(ANSI_Color16 col) {
        return static_cast<int>(col) > 7 ? static_cast<int>(col) + 30 : static_cast<int>(col) + 90;
    }
};

} // namespace ANSI
} // namespace incom::standard::console


// C++23 formatter only if compiling with C++23 or later
#if __cplusplus >= 202300L
#include <format>

namespace std {
template <>
struct formatter<incom::standard::console::ANSI::SGR_builder> {
    template <typename FormatContext>
    constexpr auto format(const incom::standard::console::ANSI::SGR_builder &obj, FormatContext &ctx) {
        return format_to(ctx.out(), obj.get_asRef());
    }
    template <typename FormatContext>
    constexpr auto format(incom::standard::console::ANSI::SGR_builder &&obj, FormatContext &ctx) {
        return format_to(ctx.out(), std::move(obj).get());
    }
};
} // namespace std
#endif