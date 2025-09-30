#pragma once
#include <string>
#include <string_view>

#include "incstd/color/color_common.hpp"


namespace incom::standard::console {
namespace ANSI {
using namespace incom::standard::color;
using namespace std::literals;


class SGR_builder {
public:
    std::string get() && { return std::move(_res); }
    std::string get() const && { return _res; }
    std::string get() & { return _res; }
    std::string get() const & { return _res; }

    const std::string &get_asRef() const & { return _res; }

    SGR_builder &add_string(std::string_view sv) {
        _res.append(sv);
        return *this;
    }


    SGR_builder reset_all() {
        _res.append("\x1b[m"sv);
        return *this;
    }

    SGR_builder color_fg(ANSI_Color16 col) {
        _res.append("\x1b["sv).append(std::to_string(_toSGR(col))).append("m");
        return *this;
    }
    SGR_builder color_fg(const std::uint8_t color_256) {
        _res.append("\x1b[38;5;").append(std::to_string(static_cast<int>(color_256))).append("m");
        return *this;
    }
    SGR_builder color_fg(const std::uint8_t r, const std::uint8_t g, const std::uint8_t b) {
        _res.append("\x1b[38;2;");
        _res.append(std::to_string(r));
        _res.push_back(';');
        _res.append(std::to_string(g));
        _res.push_back(';');
        _res.append(std::to_string(b));
        _res.push_back('m');
        return *this;
    }
    SGR_builder color_fg(const inc_sRGB &color) {
        _res.append("\x1b[38;2;");
        _res.append(std::to_string(color[0]));
        _res.push_back(';');
        _res.append(std::to_string(color[1]));
        _res.push_back(';');
        _res.append(std::to_string(color[2]));
        _res.push_back('m');
        return *this;
    }


    SGR_builder color_bg(ANSI_Color16 col) {
        _res.append("\x1b["sv).append(std::to_string(_toSGR(col) + 10)).append("m");
        return *this;
    }
    SGR_builder color_bg(const std::uint8_t color_256) {
        _res.append("\x1b[48;5;").append(std::to_string(static_cast<int>(color_256))).append("m");
        return *this;
    }
    SGR_builder color_bg(const std::uint8_t r, const std::uint8_t g, const std::uint8_t b) {
        _res.append("\x1b[48;2;");
        _res.append(std::to_string(r));
        _res.push_back(';');
        _res.append(std::to_string(g));
        _res.push_back(';');
        _res.append(std::to_string(b));
        _res.push_back('m');
        return *this;
    }
    SGR_builder color_bg(const inc_sRGB &color) {
        _res.append("\x1b[48;2;");
        _res.append(std::to_string(color[0]));
        _res.push_back(';');
        _res.append(std::to_string(color[1]));
        _res.push_back(';');
        _res.append(std::to_string(color[2]));
        _res.push_back('m');
        return *this;
    }


    SGR_builder color_fg_default() {
        _res.append("\x1b[39m"sv);
        return *this;
    }
    SGR_builder color_bg_default() {
        _res.append("\x1b[49m"sv);
        return *this;
    }


    SGR_builder bold() {
        _res.append("\x1b[1m"sv);
        return *this;
    }
    SGR_builder faint() {
        _res.append("\x1b[2m"sv);
        return *this;
    }
    SGR_builder italic() {
        _res.append("\x1b[3m"sv);
        return *this;
    }
    SGR_builder underline() {
        _res.append("\x1b[4m"sv);
        return *this;
    }
    SGR_builder underlineDouble() {
        _res.append("\x1b[21m"sv);
        return *this;
    }
    SGR_builder overline() {
        _res.append("\x1b[53m"sv);
        return *this;
    }
    SGR_builder strike() {
        _res.append("\x1b[9m"sv);
        return *this;
    }
    SGR_builder conceal() {
        _res.append("\x1b[8m"sv);
        return *this;
    }
    SGR_builder normalIntensity() {
        _res.append("\x1b[22m"sv);
        return *this;
    }
    SGR_builder notItalic() {
        _res.append("\x1b[23m"sv);
        return *this;
    }
    SGR_builder notUnderline() {
        _res.append("\x1b[24m"sv);
        return *this;
    }
    SGR_builder notOverline() {
        _res.append("\x1b[1m"sv);
        return *this;
    }
    SGR_builder notStrike() {
        _res.append("\x1b[29m"sv);
        return *this;
    }
    SGR_builder reveal() {
        _res.append("\x1b[28m"sv);
        return *this;
    }


    SGR_builder framed() {
        _res.append("\x1b[51m"sv);
        return *this;
    }
    SGR_builder encircled() {
        _res.append("\x1b[52m"sv);
        return *this;
    }
    SGR_builder notFrameEncircled() {
        _res.append("\x1b[54m"sv);
        return *this;
    }


    SGR_builder underlineColor() {
        _res.append("\x1b[59m"sv);
        return *this;
    }
    SGR_builder underlineColor_default() {
        _res.append("\x1b[59m"sv);
        return *this;
    }


    SGR_builder superscript() {
        _res.append("\x1b[73m"sv);
        return *this;
    }
    SGR_builder subscript() {
        _res.append("\x1b[74m"sv);
        return *this;
    }
    SGR_builder notSuperSubScript() {
        _res.append("\x1b[75m"sv);
        return *this;
    }


    SGR_builder colors_inverted() {
        _res.append("\x1b[7m"sv);
        return *this;
    }
    SGR_builder colors_reverted() {
        _res.append("\x1b[27m"sv);
        return *this;
    }

    SGR_builder blinkSlow() {
        _res.append("\x1b[5m"sv);
        return *this;
    }
    SGR_builder blinkFast() {
        _res.append("\x1b[6m"sv);
        return *this;
    }
    SGR_builder notBlink() {
        _res.append("\x1b[25m"sv);
        return *this;
    }


    friend std::ostream &operator<<(std::ostream &os, const SGR_builder &obj) { return os << obj.get_asRef(); }
    friend std::ostream &operator<<(std::ostream &os, SGR_builder &&obj) { return os << std::move(obj).get(); }

private:
    std::string _res;

    int _toSGR(ANSI_Color16 col) {
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
    auto format(const incom::standard::console::ANSI::SGR_builder &obj, FormatContext &ctx) {
        return format_to(ctx.out(), obj.get_asRef());
    }
    template <typename FormatContext>
    auto format(incom::standard::console::ANSI::SGR_builder &&obj, FormatContext &ctx) {
        return format_to(ctx.out(), std::move(obj).get());
    }
};
} // namespace std
#endif