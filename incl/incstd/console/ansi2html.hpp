#pragma once

#include <algorithm>
#include <array>
#include <cassert>
#include <cctype>
#include <iomanip>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "ansi_sequences.hpp"
#include "incstd/color/color_common.hpp"
#include "incstd/console/colorschemes.hpp"

namespace incom::standard::console {
using namespace incom::standard::color;
using namespace std::literals;

class AnsiToHtml {

public:
    struct Options {
        // When true, output a full HTML page <html>...; otherwise returns fragment.
        bool        full_page              = true;
        // base CSS to include in <style> when full_page == true
        std::string base_css               = R"(
body { background: #1e1e1e; color: #dcdcdc; font-family: monospace; padding: 1rem; white-space: pre-wrap; }
.term-output { white-space: pre-wrap; font-family: ui-monospace, SFMono-Regular, Menlo, Monaco, "Roboto Mono", "Liberation Mono", monospace; }
)";
        // Whether to emit <br> for newline or leave actual newlines (pre-wrap handles display).
        bool        convert_newlines_to_br = false;
        // Default foreground and background for style normalization (as CSS)
        std::string default_fg             = "#dcdcdc";
        std::string default_bg             = "transparent";

        color_schemes::scheme256 schm = color_schemes::defaultScheme256;
    };

    explicit AnsiToHtml() : opts_(Options{}) {}
    explicit AnsiToHtml(Options opts) : opts_(std::move(opts)) {}


    // Convert ANSI-containing text to HTML string.
    std::string convert(std::string_view input) {
        reset_state();
        // Reserve approx size
        std::string out;
        out.reserve(input.size() * 3 / 2 + 256);

        if (opts_.full_page) {
            out += "<!doctype html>\n<html>\n<head>\n<meta charset=\"utf-8\">\n";
            out += "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">\n";
            out += "<style>\n" + opts_.base_css + "\n</style>\n</head>\n<body>\n<div class=\"term-output\">\n";
        }

        out.append(parse_and_emit(input));

        if (opts_.full_page) { out += "\n</div>\n</body>\n</html>\n"; }
        return out;
    }

private:
    // Strongly-typed enumerations for attributes
    enum class SgrAttr {
        Reset     = 0,
        Bold      = 1,
        Dim       = 2,
        Italic    = 3,
        Underline = 4,
        Blink     = 5,
        Inverse   = 7,
        Hidden    = 8,
        Strike    = 9,
        // 21/22/23/24/25/27/28/29 are "off" variations; we handle selectively.
        Unknown
    };

    struct Styles {
        std::vector<ANSI::SGR_map> simpleSGR_;
        std::vector<std::string>   colorSGR_;
    };


    struct Style {
        std::optional<std::string> fg; // hex "#rrggbb"
        std::optional<std::string> bg;
        bool                       bold      = false;
        bool                       dim       = false;
        bool                       italic    = false;
        bool                       underline = false;
        bool                       blink     = false;
        bool                       inverse   = false;
        bool                       hidden    = false;
        bool                       strike    = false;

        bool operator==(Style const &o) const = default;
        bool is_default(const Options &opts) const {
            return ! fg && ! bg && ! bold && ! dim && ! italic && ! underline && ! blink && ! inverse && ! hidden &&
                   ! strike;
        }
    };

    Options                  opts_;
    Style                    cur_style_;
    std::string              styling;
    std::vector<std::string> hyperlink_stack_; // for OSC 8 hyperlinks

    void reset_state() {
        cur_style_ = Style{};
        hyperlink_stack_.clear();
    }

    // --- Utilities ---
    static bool is_control_char(char c) { return (unsigned char)c < 0x20 || c == 0x7f; }

    static std::string escape_html(std::string_view s) {
        std::string out;
        out.reserve(s.size());
        for (unsigned char c : s) {
            switch (c) {
                case '&': out += "&amp;"; break;
                case '<': out += "&lt;"; break;
                case '>': out += "&gt;"; break;
                case '"': out += "&quot;"; break;
                default:
                    // Keep tabs and newlines; they will be interpreted by wrapper HTML/CSS
                    out.push_back(static_cast<char>(c));
            }
        }
        return out;
    }

    // Convert 0..5 scaled channel from xterm 256 color cube to 0..255
    static uint8_t color_cube_level(int v) {
        // v in 0..5
        if (v <= 0) { return 0; }
        if (v >= 5) { return 255; }
        // mapping: 0 -> 0, 1 -> 95, 2 -> 135, 3 -> 175, 4 -> 215, 5 -> 255 (xterm palette)
        static constexpr std::array<int, 6> map = {0, 95, 135, 175, 215, 255};
        return static_cast<uint8_t>(map[v]);
    }

    static inc_sRGB xterm256_to_rgb(int n) {
        if (n >= 0 && n <= 15) {
            // Standard 16 colors — map to reasonably close  inc_sRGB values
            static constexpr std::array<inc_sRGB, 16> basic = {{{0, 0, 0},
                                                                {128, 0, 0},
                                                                {0, 128, 0},
                                                                {128, 128, 0},
                                                                {0, 0, 128},
                                                                {128, 0, 128},
                                                                {0, 128, 128},
                                                                {192, 192, 192},
                                                                {128, 128, 128},
                                                                {255, 0, 0},
                                                                {0, 255, 0},
                                                                {255, 255, 0},
                                                                {0, 0, 255},
                                                                {255, 0, 255},
                                                                {0, 255, 255},
                                                                {255, 255, 255}}};
            return basic[n];
        }
        if (n >= 16 && n <= 231) {
            int v = n - 16;
            int r = v / 36;
            int g = (v / 6) % 6;
            int b = v % 6;
            return {color_cube_level(r), color_cube_level(g), color_cube_level(b)};
        }
        if (n >= 232 && n <= 255) {
            int shade = 8 + (n - 232) * 10;
            return {(uint8_t)shade, (uint8_t)shade, (uint8_t)shade};
        }
        return {0, 0, 0};
    }

    static std::optional<inc_sRGB> parse_rgb_triplet(std::vector<int> const &params, size_t start) {
        if (start + 2 >= params.size()) { return std::nullopt; }
        int r = params[start], g = params[start + 1], b = params[start + 2];
        if (r < 0 || g < 0 || b < 0 || r > 255 || g > 255 || b > 255) { return std::nullopt; }
        return inc_sRGB{static_cast<uint8_t>(r), static_cast<uint8_t>(g), static_cast<uint8_t>(b)};
    }

    // Build CSS style string for a given Style
    std::string style_to_css(Style const &s) const {
        std::string css;
        css.reserve(64);
        if (s.inverse) {
            std::string fg  = s.bg ? *s.bg : opts_.default_fg;
            std::string bg  = s.fg ? *s.fg : opts_.default_bg;
            // swap
            css            += "color:" + fg + ";background-color:" + bg + ";";
        }
        else {
            if (s.fg) { css += "color:" + *s.fg + ";"; }
            if (s.bg) { css += "background-color:" + *s.bg + ";"; }
        }
        if (s.bold) { css += "font-weight:bold;"; }
        else if (s.dim) {
            css += "opacity:0.8;"; // visual dimming
        }
        if (s.italic) { css += "font-style:italic;"; }
        if (s.underline) { css += "text-decoration:underline;"; }
        if (s.strike) { css += "text-decoration:line-through;"; }
        if (s.hidden) { css += "visibility:hidden;"; }
        if (s.blink) {
            css += "text-decoration:blink;"; // browsers may ignore
        }
        return css;
    }

    // Emit text chunk with current styling into out, wrapping spans and hyperlink as needed.
    void emit_text_segment(std::string_view text, std::string &out) {
        if (text.empty()) { return; }
        // escape HTML
        std::string esc = escape_html(text);
        if (opts_.convert_newlines_to_br) {
            // convert \n to <br>\n
            std::string replaced;
            replaced.reserve(esc.size());
            for (size_t i = 0; i < esc.size(); ++i) {
                char c = esc[i];
                if (c == '\n') { replaced += "<br>\n"; }
                else { replaced += c; }
            }
            esc.swap(replaced);
        }
        // wrap opening hyperlink tags if any
        for (const auto &href : hyperlink_stack_) {
            out += "<a href=\"";
            out += escape_html(href);
            out += "\" target=\"_blank\" rel=\"noopener noreferrer\">";
        }

        if (cur_style_.is_default(opts_)) { out += esc; }
        else {
            std::string css  = style_to_css(cur_style_);
            out             += "<span";
            if (! css.empty()) {
                out += " style=\"";
                out += css;
                out += "\"";
            }
            out.push_back('>');
            out += esc;
            out += "</span>";
        }

        // close hyperlink tags
        for (size_t i = 0; i < hyperlink_stack_.size(); ++i) { out += "</a>"; }
    }

    // --- Parsing ---
    // Recognize ESC (0x1B) sequences. We'll handle:
    //  - CSI ... m   (SGR parameters)
    //  - OSC 8 ; params ; URI ST  (hyperlink start/end)
    //  - OSC 0/2 ... ST (window title) -> ignore
    //  - other sequences -> ignore / skip until terminator
    std::string parse_and_emit(std::string_view input) {
        std::string  out;
        size_t       i = 0, start_plain = 0;
        const size_t n = input.size();

        while (i < n) {
            char c = input[i];
            if (c == '\x1B') {
                // flush plain text up to here
                if (start_plain < i) { emit_text_segment(input.substr(start_plain, i - start_plain), out); }
                // Try to parse sequence
                if (i + 1 >= n) {
                    ++i;
                    break;
                }
                char next = input[i + 1];
                if (next == '[') {
                    // CSI - ESC [
                    size_t seq_start  = i;
                    i                += 2;
                    // read params and final byte
                    size_t j          = i;
                    // CSI parameters are bytes until a letter in @ A-Z [ \ ] ^ _ ` a-z { | } ~? but SGR uses 'm'
                    while (j < n && ! ((input[j] >= '@' && input[j] <= '~'))) { ++j; }
                    if (j >= n) { // truncated, treat as plain
                        // consume and break
                        i           = n;
                        start_plain = i;
                        break;
                    }
                    char             final_byte = input[j];
                    std::string_view seq        = input.substr(i, j - i);
                    if (final_byte == 'm') {
                        // SGR
                        auto params = split_sgr_params(seq);
                        apply_sgr(params);
                    }
                    else {
                        // Unsupported CSI final, skip (e.g., cursor movement). No effect on style.
                        // Nothing to do.
                    }
                    i           = j + 1;
                    start_plain = i;
                }
                else if (next == ']') {
                    // OSC - ESC ]
                    // Format: ESC ] <ps> ; <string> BEL  OR  ESC \  (ST)
                    // We'll scan until BEL (0x07) or ESC '\'
                    i        += 2;
                    size_t j  = i;
                    // Parameter is digits until ';'
                    // find ';'
                    while (j < n && input[j] != ';') { ++j; }
                    if (j >= n) { // malformed
                        i           = j;
                        start_plain = i;
                        continue;
                    }
                    std::string_view ps            = input.substr(i, j - i);
                    // content starts after ';'
                    size_t           content_start = j + 1;
                    size_t           content_end   = content_start;
                    bool             terminated    = false;
                    while (content_end < n) {
                        if (input[content_end] == '\x07') { // BEL
                            terminated = true;
                            break;
                        }
                        if (input[content_end] == '\x1B' && content_end + 1 < n && input[content_end + 1] == '\\') {
                            terminated = true;
                            break;
                        }
                        ++content_end;
                    }
                    std::string_view content = input.substr(content_start, content_end - content_start);
                    // handle ps
                    if (! ps.empty()) {
                        // typically ps == "8" for hyperlinks, "0" or "2" for title.
                        if (ps == "8") {
                            // hyperlnk: format "params;URI" where params often ";;" or "id"; modern form: OSC 8 ;
                            // params ; URI ST If content is empty -> terminate hyperlink
                            if (content.empty()) {
                                if (! hyperlink_stack_.empty()) { hyperlink_stack_.pop_back(); }
                            }
                            else {
                                // The content may be "params;uri" or simply URI depending on emitter. We'll take entire
                                // content as URI. But typical format is "<params>;<uri>" where <params> often empty.
                                // Try to split by ';' into at most two parts, taking last as URI.
                                std::string uri;
                                size_t      sep = content.rfind(';');
                                if (sep != std::string_view::npos) { uri = std::string(content.substr(sep + 1)); }
                                else { uri = std::string(content); }
                                hyperlink_stack_.push_back(uri);
                            }
                        }
                        else {
                            // OSC 0/2 are title-setting; ignore for HTML output.
                        }
                    }
                    // advance i past terminator
                    if (content_end < n && input[content_end] == '\x07') { i = content_end + 1; }
                    else if (content_end + 1 < n && input[content_end] == '\x1B' && input[content_end + 1] == '\\') {
                        i = content_end + 2;
                    }
                    else { i = content_end; }
                    start_plain = i;
                }
                else {
                    // Other ESC sequences (like ESC ( B, ESC M, etc.). We'll skip ESC and next char.
                    i           += 2;
                    start_plain  = i;
                }
            }
            else {
                // normal char
                ++i;
            }
        }

        // flush remainder
        if (start_plain < n) { emit_text_segment(input.substr(start_plain, n - start_plain), out); }
        return out;
    }

    // Split SGR parameters (sequence between '[' and final 'm') separated by ';'
    static std::vector<int> split_sgr_params(std::string_view seq) {
        std::vector<int> out;
        if (seq.empty()) {
            out.push_back(0); // empty SGR == reset
            return out;
        }
        size_t i = 0, n = seq.size();
        while (i < n) {
            // accept numeric (could be empty token)
            size_t j = i;
            while (j < n && seq[j] != ';') { ++j; }
            if (j == i) {
                // empty token -> treat as 0
                out.push_back(0);
            }
            else {
                // parse integer (clamp on error)
                int    val = 0;
                bool   neg = false;
                size_t k   = i;
                if (k < j && seq[k] == '-') {
                    neg = true;
                    ++k;
                }
                for (; k < j; ++k) {
                    char ch = seq[k];
                    if (ch < '0' || ch > '9') {
                        val = 0;
                        break;
                    }
                    val = val * 10 + (ch - '0');
                }
                if (neg) { val = -val; }
                out.push_back(val);
            }
            i = j + 1;
        }
        return out;
    }

    // Apply SGR parameters to current style (mutates cur_style_)
    void apply_sgr(std::vector<int> const &params) {
        if (params.empty()) {
            // reset
            styling.clear();
            return;
        }

        // If first param is 0 and others exist, treat as reset then continue
        if (params.size() == 1 && params[0] == 0) {
            styling.clear();
            return;
        }

        size_t i = 0;
        while (i < params.size()) {
            int p = params[i];
            switch (p) {
                case 0:
                    styling.clear();
                    ++i;
                    break;
                case 1:
                    styling.append("font-weight:bold;"sv);
                    ++i;
                    break;
                case 2:
                    styling.append("opacity:0.8;"sv); // visual dimming
                    ++i;
                    break;
                case 3:
                    styling.append("font-style:italic;"sv);
                    ++i;
                    break;
                case 4:
                    styling.append("text-decoration:underline;"sv);
                    ++i;
                    break;
                case 5:
                    styling.append("text-decoration:blink;"sv);
                    ++i;
                    break;
                case 7:
                    // INVERSE is unimplemented
                    ++i;
                    break;
                case 8:
                    styling.append("visibility:hidden;"sv);
                    ++i;
                    break;
                case 9:
                    styling.append("text-decoration:line-through;"sv);
                    ++i;
                    break;
                case 21:
                    styling.append("font-weight:normal;"sv); // Revert intensity to normal
                    ++i;
                    break;
                case 22:
                    styling.append("font-weight:normal;"sv); // Revert intensity to normal
                    styling.append("opacity:1;"sv);          // Revert opacity to default
                    ++i;
                    break;
                case 23:
                    styling.append("font-style:normal;"sv); // Italic off
                    ++i;
                    break;
                case 24:
                    styling.append("text-decoration-line:none;"sv); // Underline and strikethrough and blink off
                    ++i;
                    break;
                case 25:
                    styling.append("text-decoration-line:none;"sv); // Underline and strikethrough and blink off
                    ++i;
                    break;
                case 27:
                    // UN-INVERSE is unimplemented
                    ++i;
                    break;
                case 28:
                    styling.append("visibility:visible;"sv);
                    ++i;
                    break;
                case 29:
                    styling.append(
                        "text-decoration-line:none;"sv); // Underline and strikethrough and blink and crossout off
                    ++i;
                    break;

                // Foreground 30-37 standard colors
                case 30:
                case 31:
                case 32:
                case 33:
                case 34:
                case 35:
                case 36:
                case 37:
                    styling.append("color:"sv);
                    styling.append(opts_.schm.palette[i - 30].to_hex());
                    styling.push_back(';');
                    ++i;
                    break;
                case 39:
                    styling.append("color:inherit;"sv);
                    ++i;
                    break; // default fg

                // Background 40-47
                case 40:
                case 41:
                case 42:
                case 43:
                case 44:
                case 45:
                case 46:
                case 47:
                    styling.append("background-color:"sv);
                    styling.append(opts_.schm.palette[i - 40].to_hex());
                    styling.push_back(';');
                    ++i;
                    break;
                case 49:
                    styling.append("background-color:inherit;"sv);
                    ++i;
                    break; // default bg

                // Bright fg 90-97
                case 90:
                case 91:
                case 92:
                case 93:
                case 94:
                case 95:
                case 96:
                case 97:
                    styling.append("color:"sv);
                    styling.append(opts_.schm.palette[i - 82].to_hex());
                    styling.push_back(';');
                    ++i;
                    break;

                // Bright bg 100-107
                case 100:
                case 101:
                case 102:
                case 103:
                case 104:
                case 105:
                case 106:
                case 107:
                    styling.append("background-color:"sv);
                    styling.append(opts_.schm.palette[i - 92].to_hex());
                    styling.push_back(';');
                    ++i;
                    break;

                // Extended colors: 38;5;<n> or 48;5;<n> or 38;2;r;g;b or 48;2;r;g;b
                case 38:
                case 48:
                    {
                        bool is_fg = (p == 38);
                        // lookahead
                        if (i + 1 < params.size()) {
                            int mode = params[i + 1];
                            if (mode == 5) {
                                // 256 color: next is index
                                if (i + 2 < params.size()) {
                                    int idx = params[i + 2];
                                    if (idx >= 0 && idx <= 255) {
                                        if (is_fg) {
                                            styling.append("color:"sv);
                                            styling.append(opts_.schm.palette[idx].to_hex());
                                            styling.push_back(';');
                                        }
                                        else {
                                            styling.append("background-color:"sv);
                                            styling.append(opts_.schm.palette[idx].to_hex());
                                            styling.push_back(';');
                                        }
                                    }
                                    else {
                                        // ignore invalid
                                    }
                                    i += 3;
                                }
                                else { i += 2; }
                            }
                            else if (mode == 2) {
                                // truecolor: r;g;b
                                if (i + 4 < params.size()) {
                                    auto maybe = parse_rgb_triplet(params, i + 2);
                                    if (maybe) {
                                        if (is_fg) {
                                            styling.append("color:"sv);
                                            styling.append(maybe->to_hex());
                                            styling.push_back(';');
                                        }
                                        else {
                                            styling.append("background-color:"sv);
                                            styling.append(maybe->to_hex());
                                            styling.push_back(';');
                                        }
                                    }
                                    i += 5;
                                }
                                else {
                                    // incomplete; jump to end
                                    i = params.size();
                                }
                            }
                            else {
                                // unsupported submode
                                i += 2;
                            }
                        }
                        else { i += 1; }
                    }
                    break;

                default:
                    // Unknown/unsupported SGR code — ignore
                    ++i;
                    break;
            }
        }
    }

    // Map standard 0..15 color indices to hex
    static std::string standard_index_to_hex(int idx) {
        // Using a common palette that approximates xterm default colors
        static constexpr std::array<inc_sRGB, 16> pal = {{{0x00, 0x00, 0x00},
                                                          {0x80, 0x00, 0x00},
                                                          {0x00, 0x80, 0x00},
                                                          {0x80, 0x80, 0x00},
                                                          {0x00, 0x00, 0x80},
                                                          {0x80, 0x00, 0x80},
                                                          {0x00, 0x80, 0x80},
                                                          {0xc0, 0xc0, 0xc0},
                                                          {0x80, 0x80, 0x80},
                                                          {0xff, 0x00, 0x00},
                                                          {0x00, 0xff, 0x00},
                                                          {0xff, 0xff, 0x00},
                                                          {0x00, 0x00, 0xff},
                                                          {0xff, 0x00, 0xff},
                                                          {0x00, 0xff, 0xff},
                                                          {0xff, 0xff, 0xff}}};
        if (idx < 0) { idx = 0; }
        if (idx > 15) { idx = 15; }

        return pal[idx].to_hex();
    }

    void set_standard_color_fg(int idx) { cur_style_.fg = standard_index_to_hex(idx); }
    void set_standard_color_bg(int idx) { cur_style_.bg = standard_index_to_hex(idx); }
};

} // namespace incom::standard::console