#pragma once

#include <cassert>
#include <expected>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <incstd/console/colorschemes.hpp>

namespace incom::standard::console {
using namespace incom::standard::color;
using namespace std::literals;

class AnsiToHtml {

public:
    struct Options {
        // When true, output a full HTML page <html>...; otherwise returns fragment.
        bool full_page = true;

        // base CSS to include in <style> when full_page == true
        std::string plotCSS =
            R"(term-output { white-space: pre-wrap; font-family: ui-monospace, SFMono-Regular, Menlo, Monaco, "Roboto Mono", "Liberation Mono", monospace; })";

        // Whether to emit <br> for newline or leave actual newlines (pre-wrap handles display).
        bool                     convert_newlines_to_br = false;
        color_schemes::scheme256 schm                   = color_schemes::defaultScheme256;
    };

    explicit AnsiToHtml() {}
    explicit AnsiToHtml(Options opts) : opts_(std::move(opts)) {}


    // Convert ANSI-containing text to HTML string.
    std::string convert(std::string_view input) {
        styling_ = _Styling_{};
        hyperlink_stack_.clear();

        // Reserve approx size
        std::string out;
        out.reserve(input.size() * 8);

        if (opts_.full_page) {
            out += "<!doctype html>\n<html>\n<head>\n<meta charset=\"utf-8\"/>\n"sv;
            out += "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\"/>\n"sv;
            out += "<style>\n" + opts_.plotCSS + "\n</style>\n</head>\n<body>\n<pre class=\"term-output\">\n";
        }

        out.append(parse_and_emit(input));

        if (opts_.full_page) { out += "\n</pre>\n</body>\n</html>\n"; }
        return out;
    }

private:
    struct _Styling_ {
        std::optional<std::string> fg;
        std::optional<std::string> bg;

        std::optional<std::string_view> opacity;
        std::optional<std::string_view> fontWidth;
        std::optional<std::string_view> fontStyle;
        std::optional<std::string_view> visibility;

        std::optional<std::string_view> blink;
        std::optional<std::string_view> underline;
        std::optional<std::string_view> strikeThrough;
        std::optional<std::string_view> textDecorNone;


        std::optional<std::string> build_stylingString() {
            std::string res;
            if (fg) { res.append(fg.value()); }
            if (bg) { res.append(bg.value()); }
            if (opacity) { res.append(opacity.value()); }
            if (fontWidth) { res.append(fontWidth.value()); }
            if (fontStyle) { res.append(fontStyle.value()); }
            if (visibility) { res.append(visibility.value()); }
            if (blink) { res.append(blink.value()); }
            if (underline) { res.append(underline.value()); }
            if (strikeThrough) { res.append(strikeThrough.value()); }
            if (textDecorNone) { res.append(textDecorNone.value()); }

            if (res.empty()) { return std::nullopt; }
            else { return res; }
            std::unreachable();
        }
    };


    Options                  opts_;
    _Styling_                styling_;
    std::vector<std::string> hyperlink_stack_; // for OSC 8 hyperlinks

    // Emit text chunk with current styling into out, wrapping spans and hyperlink as needed.
    void emit_text_segment(std::string_view text, std::string &out) {
        if (text.empty()) { return; }

        // escape HTML
        auto escapeHTML = [](std::string_view s) -> std::string {
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
        };

        std::string esc = escapeHTML(text);
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
            out += escapeHTML(href);
            out += "\" target=\"_blank\" rel=\"noopener noreferrer\">";
        }


        if (auto style = styling_.build_stylingString(); not style.has_value()) { out += esc; }
        else {
            out += "<span";

            out += " style=\"";
            out += style.value();
            out.push_back('\"');

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
        std::string out;
        out.reserve(input.size() * 8);
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
                        // Split SGR parameters (sequence between '[' and final 'm') separated by ';'
                        auto splitSgrParams = [](std::string_view seq) -> std::vector<int> {
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
                        };
                        // SGR
                        auto params = splitSgrParams(seq);
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

    // Apply SGR parameters to current style (mutates styling)
    void apply_sgr(std::vector<int> const &params) {
        if (params.empty()) {
            // reset
            styling_ = _Styling_{};
            return;
        }

        size_t i = 0;
        while (i < params.size()) {
            int p = params[i];
            switch (p) {
                case 0:
                    styling_ = _Styling_{};
                    ++i;
                    break;
                case 1:
                    styling_.fontWidth = "font-weight:bold;"sv;
                    ++i;
                    break;
                case 2:
                    styling_.opacity = "opacity:0.8;"sv;
                    ++i;
                    break;
                case 3:
                    styling_.fontStyle = "font-style:italic;"sv;
                    ++i;
                    break;
                case 4:
                    styling_.underline     = "text-decoration:underline;"sv;
                    styling_.textDecorNone = std::nullopt;
                    ++i;
                    break;
                case 5:
                    styling_.blink         = "text-decoration:blink;"sv;
                    styling_.textDecorNone = std::nullopt;
                    ++i;
                    break;
                case 7:
                    // INVERSE is unimplemented
                    ++i;
                    break;
                case 8:
                    styling_.visibility = "visibility:hidden;"sv;
                    ++i;
                    break;
                case 9:
                    styling_.strikeThrough = "text-decoration:line-through;"sv;
                    styling_.textDecorNone = std::nullopt;
                    ++i;
                    break;
                case 21:
                    styling_.fontWidth = "font-weight:normal;"sv;
                    ++i;
                    break;
                case 22:
                    styling_.fontWidth = "font-weight:normal;"sv;
                    styling_.opacity   = "opacity:1;"sv;
                    ++i;
                    break;
                case 23:
                    styling_.fontStyle = "font-style:normal;"sv; // Italic off
                    ++i;
                    break;
                case 24:
                    styling_.underline     = std::nullopt;
                    styling_.blink         = std::nullopt;
                    styling_.strikeThrough = std::nullopt;
                    styling_.textDecorNone =
                        "text-decoration-line:none;"sv; // Underline and strikethrough and blink off
                    ++i;
                    break;
                case 25:
                    styling_.underline     = std::nullopt;
                    styling_.blink         = std::nullopt;
                    styling_.strikeThrough = std::nullopt;
                    styling_.textDecorNone =
                        "text-decoration-line:none;"sv; // Underline and strikethrough and blink off
                    ++i;
                    break;
                case 27:
                    // UN-INVERSE is unimplemented
                    ++i;
                    break;
                case 28:
                    styling_.visibility = "visibility:visible;"sv;
                    ++i;
                    break;
                case 29:
                    styling_.underline     = std::nullopt;
                    styling_.blink         = std::nullopt;
                    styling_.strikeThrough = std::nullopt;
                    styling_.textDecorNone =
                        "text-decoration-line:none;"sv; // Underline and strikethrough and blink off
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
                    styling_.fg = std::string("color:"sv);
                    styling_.fg->append(opts_.schm.palette[p - 30].to_hex());
                    styling_.fg->push_back(';');
                    ++i;
                    break;
                case 39:
                    styling_.fg = std::string("color:"sv);
                    styling_.fg->append(opts_.schm.foreground.to_hex());
                    styling_.fg->push_back(';');
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
                    styling_.fg = std::string("background-color:"sv);
                    styling_.fg->append(opts_.schm.palette[p - 40].to_hex());
                    styling_.fg->push_back(';');
                    ++i;
                    break;
                case 49:
                    styling_.bg = std::string("color:"sv);
                    styling_.bg->append(opts_.schm.backgrond.to_hex());
                    styling_.bg->push_back(';');
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
                    styling_.fg = std::string("color:"sv);
                    styling_.fg->append(opts_.schm.palette[p - 82].to_hex());
                    styling_.fg->push_back(';');
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
                    styling_.bg = std::string("background-color:"sv);
                    styling_.bg->append(opts_.schm.palette[p - 92].to_hex());
                    styling_.bg->push_back(';');
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
                                            styling_.fg = std::string("color:"sv);
                                            styling_.fg->append(opts_.schm.palette[idx].to_hex());
                                            styling_.fg->push_back(';');
                                        }
                                        else {
                                            styling_.bg = std::string("background-color:"sv);
                                            styling_.bg->append(opts_.schm.palette[idx].to_hex());
                                            styling_.bg->push_back(';');
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
                                    auto parse_rgb_triplet = [](std::vector<int> const &params,
                                                                size_t start) -> std::optional<inc_sRGB> {
                                        if (start + 2 >= params.size()) { return std::nullopt; }
                                        int r = params[start], g = params[start + 1], b = params[start + 2];
                                        if (r < 0 || g < 0 || b < 0 || r > 255 || g > 255 || b > 255) {
                                            return std::nullopt;
                                        }
                                        return inc_sRGB{static_cast<uint8_t>(r), static_cast<uint8_t>(g),
                                                        static_cast<uint8_t>(b)};
                                    };
                                    auto maybe = parse_rgb_triplet(params, i + 2);
                                    if (maybe) {
                                        if (is_fg) {
                                            styling_.fg = std::string("color:"sv);
                                            styling_.fg->append(maybe->to_hex());
                                            styling_.fg->push_back(';');
                                        }
                                        else {
                                            styling_.bg = std::string("background-color:"sv);
                                            styling_.bg->append(maybe->to_hex());
                                            styling_.bg->push_back(';');
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
};

} // namespace incom::standard::console