#pragma once

#include <cstddef>
#include <optional>
#include <string>

#include <incstd/color/color_common.hpp>

namespace incom::standard::console::color_schemes {
using namespace incom::standard::color;

struct scheme16 {
    std::optional<std::string> name = std::nullopt;
    palette16                  palette;
    inc_sRGB                   foreground;
    inc_sRGB                   backgrond;
    inc_sRGB                   cursor;
    inc_sRGB                   selection;

    inc_sRGB const &get_nonBWColor(size_t id) const { return palette.at(id + 1 + (id > 7)); }
};
struct scheme256 {
    std::optional<std::string> name = std::nullopt;
    palette256                 palette;
    inc_sRGB                   foreground;
    inc_sRGB                   backgrond;
    inc_sRGB                   cursor;
    inc_sRGB                   selection;

    inc_sRGB const &get_nonBWColor(size_t id) const { return palette.at(id > 11 ? id : (id + 1 + (id > 7))); }
};

inline constexpr scheme16 conv_s256s16(const scheme256 &from) {
    return scheme16{.name = from.name,
                    .palette{from.palette[0], from.palette[1], from.palette[2], from.palette[3], from.palette[4],
                             from.palette[5], from.palette[6], from.palette[7], from.palette[8], from.palette[9],
                             from.palette[10], from.palette[11], from.palette[12], from.palette[13], from.palette[14],
                             from.palette[15]},
                    .foreground{from.foreground},
                    .backgrond{from.backgrond},
                    .cursor{from.cursor},
                    .selection{from.selection}};
}

inline constexpr scheme256 conv_s16s256(const scheme16 &from) {
    scheme256 res{.name = from.name,
                  .palette{from.palette[0], from.palette[1], from.palette[2], from.palette[3], from.palette[4],
                           from.palette[5], from.palette[6], from.palette[7], from.palette[8], from.palette[9],
                           from.palette[10], from.palette[11], from.palette[12], from.palette[13], from.palette[14],
                           from.palette[15]},
                  .foreground{from.foreground},
                  .backgrond{from.backgrond},
                  .cursor{from.cursor},
                  .selection{from.selection}};
    for (size_t i = 16; i < 256; ++i) { res.palette[i] = inc_sRGB{255, 255, 255}; }
    return res;
}

inline constexpr int get_SGR_fg(ANSI_Color16 col) {
    constexpr std::array<int, 16> map{30, 31, 32, 33, 34, 35, 36, 37, 90, 91, 92, 93, 94, 95, 96, 97};
    return map[static_cast<int>(col)];
}
inline constexpr int get_SGR_bg(ANSI_Color16 col) {
    constexpr std::array<int, 16> map{40, 41, 42, 43, 44, 45, 46, 47, 100, 101, 102, 103, 104, 105, 106, 107};
    return map[static_cast<int>(col)];
}


namespace windows_terminal {

inline constexpr scheme256 campbell256{
    "campbell",
    {{{12, 12, 12},    {197, 15, 31},   {19, 161, 14},   {193, 156, 0},   {0, 55, 218},    {136, 23, 152},
      {58, 150, 221},  {204, 204, 204}, {118, 118, 118}, {231, 72, 86},   {22, 198, 12},   {249, 241, 165},
      {59, 120, 255},  {180, 0, 158},   {97, 214, 214},  {242, 242, 242}, {0, 0, 0},       {0, 0, 95},
      {0, 0, 135},     {0, 0, 175},     {0, 0, 215},     {0, 0, 255},     {0, 95, 0},      {0, 95, 95},
      {0, 95, 135},    {0, 95, 175},    {0, 95, 215},    {0, 95, 255},    {0, 135, 0},     {0, 135, 95},
      {0, 135, 135},   {0, 135, 175},   {0, 135, 215},   {0, 135, 255},   {0, 175, 0},     {0, 175, 95},
      {0, 175, 135},   {0, 175, 175},   {0, 175, 215},   {0, 175, 255},   {0, 215, 0},     {0, 215, 95},
      {0, 215, 135},   {0, 215, 175},   {0, 215, 215},   {0, 215, 255},   {0, 255, 0},     {0, 255, 95},
      {0, 255, 135},   {0, 255, 175},   {0, 255, 215},   {0, 255, 255},   {95, 0, 0},      {95, 0, 95},
      {95, 0, 135},    {95, 0, 175},    {95, 0, 215},    {95, 0, 255},    {95, 95, 0},     {95, 95, 95},
      {95, 95, 135},   {95, 95, 175},   {95, 95, 215},   {95, 95, 255},   {95, 135, 0},    {95, 135, 95},
      {95, 135, 135},  {95, 135, 175},  {95, 135, 215},  {95, 135, 255},  {95, 175, 0},    {95, 175, 95},
      {95, 175, 135},  {95, 175, 175},  {95, 175, 215},  {95, 175, 255},  {95, 215, 0},    {95, 215, 95},
      {95, 215, 135},  {95, 215, 175},  {95, 215, 215},  {95, 215, 255},  {95, 255, 0},    {95, 255, 95},
      {95, 255, 135},  {95, 255, 175},  {95, 255, 215},  {95, 255, 255},  {135, 0, 0},     {135, 0, 95},
      {135, 0, 135},   {135, 0, 175},   {135, 0, 215},   {135, 0, 255},   {135, 95, 0},    {135, 95, 95},
      {135, 95, 135},  {135, 95, 175},  {135, 95, 215},  {135, 95, 255},  {135, 135, 0},   {135, 135, 95},
      {135, 135, 135}, {135, 135, 175}, {135, 135, 215}, {135, 135, 255}, {135, 175, 0},   {135, 175, 95},
      {135, 175, 135}, {135, 175, 175}, {135, 175, 215}, {135, 175, 255}, {135, 215, 0},   {135, 215, 95},
      {135, 215, 135}, {135, 215, 175}, {135, 215, 215}, {135, 215, 255}, {135, 255, 0},   {135, 255, 95},
      {135, 255, 135}, {135, 255, 175}, {135, 255, 215}, {135, 255, 255}, {175, 0, 0},     {175, 0, 95},
      {175, 0, 135},   {175, 0, 175},   {175, 0, 215},   {175, 0, 255},   {175, 95, 0},    {175, 95, 95},
      {175, 95, 135},  {175, 95, 175},  {175, 95, 215},  {175, 95, 255},  {175, 135, 0},   {175, 135, 95},
      {175, 135, 135}, {175, 135, 175}, {175, 135, 215}, {175, 135, 255}, {175, 175, 0},   {175, 175, 95},
      {175, 175, 135}, {175, 175, 175}, {175, 175, 215}, {175, 175, 255}, {175, 215, 0},   {175, 215, 95},
      {175, 215, 135}, {175, 215, 175}, {175, 215, 215}, {175, 215, 255}, {175, 255, 0},   {175, 255, 95},
      {175, 255, 135}, {175, 255, 175}, {175, 255, 215}, {175, 255, 255}, {215, 0, 0},     {215, 0, 95},
      {215, 0, 135},   {215, 0, 175},   {215, 0, 215},   {215, 0, 255},   {215, 95, 0},    {215, 95, 95},
      {215, 95, 135},  {215, 95, 175},  {215, 95, 215},  {215, 95, 255},  {215, 135, 0},   {215, 135, 95},
      {215, 135, 135}, {215, 135, 175}, {215, 135, 215}, {215, 135, 255}, {215, 175, 0},   {215, 175, 95},
      {215, 175, 135}, {215, 175, 175}, {215, 175, 215}, {215, 175, 255}, {215, 215, 0},   {215, 215, 95},
      {215, 215, 135}, {215, 215, 175}, {215, 215, 215}, {215, 215, 255}, {215, 255, 0},   {215, 255, 95},
      {215, 255, 135}, {215, 255, 175}, {215, 255, 215}, {215, 255, 255}, {255, 0, 0},     {255, 0, 95},
      {255, 0, 135},   {255, 0, 175},   {255, 0, 215},   {255, 0, 255},   {255, 95, 0},    {255, 95, 95},
      {255, 95, 135},  {255, 95, 175},  {255, 95, 215},  {255, 95, 255},  {255, 135, 0},   {255, 135, 95},
      {255, 135, 135}, {255, 135, 175}, {255, 135, 215}, {255, 135, 255}, {255, 175, 0},   {255, 175, 95},
      {255, 175, 135}, {255, 175, 175}, {255, 175, 215}, {255, 175, 255}, {255, 215, 0},   {255, 215, 95},
      {255, 215, 135}, {255, 215, 175}, {255, 215, 215}, {255, 215, 255}, {255, 255, 0},   {255, 255, 95},
      {255, 255, 135}, {255, 255, 175}, {255, 255, 215}, {255, 255, 255}, {8, 8, 8},       {18, 18, 18},
      {28, 28, 28},    {38, 38, 38},    {48, 48, 48},    {58, 58, 58},    {68, 68, 68},    {78, 78, 78},
      {88, 88, 88},    {98, 98, 98},    {108, 108, 108}, {118, 118, 118}, {128, 128, 128}, {138, 138, 138},
      {148, 148, 148}, {158, 158, 158}, {168, 168, 168}, {178, 178, 178}, {188, 188, 188}, {198, 198, 198},
      {208, 208, 208}, {218, 218, 218}, {228, 228, 228}, {238, 238, 238}}},
    {204, 204, 204},
    {12, 12, 12},
    {255, 255, 255},
    {255, 255, 255}};


inline constexpr scheme16 campbell{
    "campbell",
    {{inc_sRGB{12, 12, 12}, inc_sRGB{197, 15, 31}, inc_sRGB{19, 161, 14}, inc_sRGB{193, 156, 0}, inc_sRGB{0, 55, 218},
      inc_sRGB{136, 23, 152}, inc_sRGB{58, 150, 221}, inc_sRGB{204, 204, 204}, inc_sRGB{118, 118, 118},
      inc_sRGB{231, 72, 86}, inc_sRGB{22, 198, 12}, inc_sRGB{249, 241, 165}, inc_sRGB{59, 120, 255},
      inc_sRGB{180, 0, 158}, inc_sRGB{97, 214, 214}, inc_sRGB{242, 242, 242}}},
    {204, 204, 204},
    {12, 12, 12},
    {255, 255, 255},
    {255, 255, 255}};

// https://github.com/dofuuz/dimidium
inline constexpr scheme16 dimidium{
    "dimidium",
    {
        // ANSI 0–15 colors (from Dimidium .itermcolors)
        inc_sRGB{0, 0, 0},       // 0  black
        inc_sRGB{207, 73, 76},   // 1  red
        inc_sRGB{96, 180, 66},   // 2  green
        inc_sRGB{219, 156, 17},  // 3  yellow
        inc_sRGB{5, 117, 216},   // 4  blue
        inc_sRGB{175, 94, 210},  // 5  magenta
        inc_sRGB{29, 182, 187},  // 6  cyan
        inc_sRGB{186, 183, 182}, // 7  white (normal)
        inc_sRGB{129, 126, 126}, // 8  bright black (grey)
        inc_sRGB{255, 100, 59},  // 9  bright red
        inc_sRGB{55, 229, 123},  // 10 bright green
        inc_sRGB{252, 205, 26},  // 11 bright yellow
        inc_sRGB{104, 141, 253}, // 12 bright blue
        inc_sRGB{237, 111, 233}, // 13 bright magenta
        inc_sRGB{50, 224, 251},  // 14 bright cyan
        inc_sRGB{222, 227, 228}  // 15 bright white
    },
    {186, 183, 182},             // foreground (from .itermcolors)
    {20, 20, 20},                // background (from .itermcolors)
    {55, 229, 123},              // cursor (from .itermcolors)
    {141, 184, 229}              // selection (from .itermcolors)
};

inline constexpr scheme16 dark_plus{
    "dark_plus",
    {{inc_sRGB{0, 0, 0}, inc_sRGB{198, 47, 55}, inc_sRGB{55, 190, 120}, inc_sRGB{226, 232, 34}, inc_sRGB{57, 110, 199},
      inc_sRGB{184, 53, 188}, inc_sRGB{59, 167, 204}, inc_sRGB{229, 229, 229}, inc_sRGB{102, 102, 102},
      inc_sRGB{233, 74, 81}, inc_sRGB{69, 211, 138}, inc_sRGB{242, 248, 74}, inc_sRGB{78, 138, 233},
      inc_sRGB{210, 106, 214}, inc_sRGB{73, 183, 218}, inc_sRGB{229, 229, 229}}},
    {204, 204, 204}, // foreground
    {30, 30, 30},    // background
    {204, 204, 204}, // cursor
    {204, 204, 204}  // selection
};

inline constexpr scheme16 vintage{
    "vintage",
    {{inc_sRGB{0, 0, 0}, inc_sRGB{128, 0, 0}, inc_sRGB{0, 128, 0}, inc_sRGB{128, 128, 0}, inc_sRGB{0, 0, 128},
      inc_sRGB{128, 0, 128}, inc_sRGB{0, 128, 128}, inc_sRGB{192, 192, 192}, inc_sRGB{128, 128, 128},
      inc_sRGB{255, 0, 0}, inc_sRGB{0, 255, 0}, inc_sRGB{255, 255, 0}, inc_sRGB{0, 0, 255}, inc_sRGB{255, 0, 255},
      inc_sRGB{0, 255, 255}, inc_sRGB{255, 255, 255}}},
    {192, 192, 192}, // foreground
    {0, 0, 0},       // background
    {192, 192, 192}, // cursor (fallback)
    {192, 192, 192}  // selection (fallback)
};

inline constexpr scheme16 ottosson{
    "ottosson",
    {{inc_sRGB{0, 38, 26}, inc_sRGB{255, 0, 0}, inc_sRGB{0, 255, 0}, inc_sRGB{255, 255, 0}, inc_sRGB{0, 0, 255},
      inc_sRGB{255, 0, 255}, inc_sRGB{0, 255, 255}, inc_sRGB{255, 255, 255}, inc_sRGB{85, 85, 85},
      inc_sRGB{255, 85, 85}, inc_sRGB{85, 255, 85}, inc_sRGB{255, 255, 85}, inc_sRGB{85, 85, 255},
      inc_sRGB{255, 85, 255}, inc_sRGB{85, 255, 255}, inc_sRGB{255, 255, 255}}},
    {255, 255, 255}, // foreground
    {0, 38, 26},     // background
    {255, 255, 255}, // cursor
    {0, 85, 51}      // selection (fallback greenish)
};

inline constexpr scheme16 one_half_dark{
    "one_half_dark",
    {{inc_sRGB{40, 44, 52}, inc_sRGB{224, 108, 117}, inc_sRGB{152, 195, 121}, inc_sRGB{229, 192, 123},
      inc_sRGB{97, 175, 239}, inc_sRGB{198, 120, 221}, inc_sRGB{86, 182, 194}, inc_sRGB{220, 223, 228},
      inc_sRGB{92, 99, 112}, inc_sRGB{224, 108, 117}, inc_sRGB{152, 195, 121}, inc_sRGB{229, 192, 123},
      inc_sRGB{97, 175, 239}, inc_sRGB{198, 120, 221}, inc_sRGB{86, 182, 194}, inc_sRGB{220, 223, 228}}},
    {220, 223, 228},
    {40, 44, 52},
    {220, 223, 228},
    {220, 223, 228}};

inline constexpr scheme16 one_half_light{
    "one_half_light",
    {{inc_sRGB{250, 250, 250}, inc_sRGB{224, 108, 117}, inc_sRGB{152, 195, 121}, inc_sRGB{184, 173, 104},
      inc_sRGB{97, 175, 239}, inc_sRGB{198, 120, 221}, inc_sRGB{86, 182, 194}, inc_sRGB{56, 58, 66},
      inc_sRGB{153, 153, 153}, inc_sRGB{224, 108, 117}, inc_sRGB{152, 195, 121}, inc_sRGB{184, 173, 104},
      inc_sRGB{97, 175, 239}, inc_sRGB{198, 120, 221}, inc_sRGB{86, 182, 194}, inc_sRGB{56, 58, 66}}},
    {56, 58, 66},
    {250, 250, 250},
    {56, 58, 66},
    {56, 58, 66}};

inline constexpr scheme16 solarized_dark{
    "solarized_dark",
    {{inc_sRGB{0, 43, 54}, inc_sRGB{220, 50, 47}, inc_sRGB{133, 153, 0}, inc_sRGB{181, 137, 0}, inc_sRGB{38, 139, 210},
      inc_sRGB{211, 54, 130}, inc_sRGB{42, 161, 152}, inc_sRGB{238, 232, 213}, inc_sRGB{88, 110, 117},
      inc_sRGB{203, 75, 22}, inc_sRGB{88, 110, 117}, inc_sRGB{101, 123, 131}, inc_sRGB{131, 148, 150},
      inc_sRGB{108, 113, 196}, inc_sRGB{147, 161, 161}, inc_sRGB{253, 246, 227}}},
    {131, 148, 150},
    {0, 43, 54},
    {131, 148, 150},
    {131, 148, 150}};

inline constexpr scheme16 solarized_light{
    "solarized_light",
    {{inc_sRGB{253, 246, 227}, inc_sRGB{220, 50, 47}, inc_sRGB{133, 153, 0}, inc_sRGB{181, 137, 0},
      inc_sRGB{38, 139, 210}, inc_sRGB{211, 54, 130}, inc_sRGB{42, 161, 152}, inc_sRGB{7, 54, 66},
      inc_sRGB{238, 232, 213}, inc_sRGB{203, 75, 22}, inc_sRGB{88, 110, 117}, inc_sRGB{101, 123, 131},
      inc_sRGB{131, 148, 150}, inc_sRGB{108, 113, 196}, inc_sRGB{147, 161, 161}, inc_sRGB{0, 43, 54}}},
    {101, 123, 131},
    {253, 246, 227},
    {101, 123, 131},
    {101, 123, 131}};

inline constexpr scheme16 tango_dark{
    "tango_dark",
    {{inc_sRGB{0, 0, 0}, inc_sRGB{204, 0, 0}, inc_sRGB{78, 154, 6}, inc_sRGB{196, 160, 0}, inc_sRGB{52, 101, 164},
      inc_sRGB{117, 80, 123}, inc_sRGB{6, 152, 154}, inc_sRGB{211, 215, 207}, inc_sRGB{85, 87, 83},
      inc_sRGB{239, 41, 41}, inc_sRGB{138, 226, 52}, inc_sRGB{252, 233, 79}, inc_sRGB{114, 159, 207},
      inc_sRGB{173, 127, 168}, inc_sRGB{52, 226, 226}, inc_sRGB{238, 238, 236}}},
    {238, 238, 238},
    {34, 34, 34},
    {238, 238, 238},
    {238, 238, 238}};

inline constexpr scheme16 tango_light{
    "tango_light",
    {{inc_sRGB{0, 0, 0}, inc_sRGB{204, 0, 0}, inc_sRGB{78, 154, 6}, inc_sRGB{196, 160, 0}, inc_sRGB{52, 101, 164},
      inc_sRGB{117, 80, 123}, inc_sRGB{6, 152, 154}, inc_sRGB{211, 215, 207}, inc_sRGB{85, 87, 83},
      inc_sRGB{239, 41, 41}, inc_sRGB{138, 226, 52}, inc_sRGB{252, 233, 79}, inc_sRGB{114, 159, 207},
      inc_sRGB{173, 127, 168}, inc_sRGB{52, 226, 226}, inc_sRGB{238, 238, 236}}},
    {56, 58, 66},
    {255, 255, 255},
    {56, 58, 66},
    {56, 58, 66}};

inline constexpr scheme16 cga{
    "cga",
    {{inc_sRGB{0, 0, 0}, inc_sRGB{0, 0, 170}, inc_sRGB{0, 170, 0}, inc_sRGB{0, 170, 170}, inc_sRGB{170, 0, 0},
      inc_sRGB{170, 0, 170}, inc_sRGB{170, 85, 0}, inc_sRGB{170, 170, 170}, inc_sRGB{85, 85, 85}, inc_sRGB{85, 85, 255},
      inc_sRGB{85, 255, 85}, inc_sRGB{85, 255, 255}, inc_sRGB{255, 85, 85}, inc_sRGB{255, 85, 255},
      inc_sRGB{255, 255, 85}, inc_sRGB{255, 255, 255}}},
    {170, 170, 170}, // foreground
    {0, 0, 0},       // background
    {170, 170, 170}, // cursor
    {85, 85, 85}     // selection (fallback)
};

inline constexpr scheme16 ibm_5153{
    "ibm_5153",
    cga.palette,     // palette identical to CGA
    {170, 170, 170}, // foreground
    {0, 0, 0},       // background
    {170, 170, 170}, // cursor
    {85, 85, 85}     // selection (fallback)
};

inline constexpr scheme16 campbell_powershell{
    "campbell_ps",
    {{inc_sRGB{12, 12, 12}, inc_sRGB{197, 15, 31}, inc_sRGB{19, 161, 14}, inc_sRGB{193, 156, 0}, inc_sRGB{0, 55, 218},
      inc_sRGB{136, 23, 152}, inc_sRGB{58, 150, 221}, inc_sRGB{204, 204, 204}, inc_sRGB{118, 118, 118},
      inc_sRGB{231, 72, 86}, inc_sRGB{22, 198, 12}, inc_sRGB{249, 241, 165}, inc_sRGB{59, 120, 255},
      inc_sRGB{180, 0, 158}, inc_sRGB{97, 214, 214}, inc_sRGB{242, 242, 242}}},
    {204, 204, 204}, // foreground
    {1, 36, 86},     // background
    {255, 255, 255}, // cursor
    {255, 255, 255}  // selection
};
} // namespace windows_terminal

namespace other_sources {
inline constexpr scheme16 monochrome{
    "monochrome",
    {
        // ANSI 0–15 colors (monochrome)
        inc_sRGB{13, 13, 13},    //

        inc_sRGB{149, 149, 149}, //
        inc_sRGB{209, 209, 209}, //
        inc_sRGB{89, 89, 89},    //
        inc_sRGB{179, 179, 179}, //
        inc_sRGB{59, 59, 59},    //
        inc_sRGB{119, 119, 119}, //

        inc_sRGB{242, 242, 242}, //


        inc_sRGB{89, 89, 89},    //

        inc_sRGB{162, 162, 162}, //
        inc_sRGB{222, 222, 222}, //
        inc_sRGB{102, 102, 102}, //
        inc_sRGB{192, 192, 192}, //
        inc_sRGB{72, 72, 72},    //
        inc_sRGB{132, 132, 132}, //

        inc_sRGB{255, 255, 255}  //
    },
    {242, 242, 242},             // foreground (#F2F2F2)
    {13, 13, 13},                // background (#0D0D0D)
    {242, 242, 242},             // cursor (set to match foreground for monochrome)
    {89, 89, 89}                 // selection (using muted gray for contrast)
};
} // namespace other_sources

inline constexpr scheme16 const  &defaultScheme16  = windows_terminal::dimidium;
inline constexpr scheme256 const &defaultScheme256 = windows_terminal::campbell256;
} // namespace incom::standard::console::color_schemes
