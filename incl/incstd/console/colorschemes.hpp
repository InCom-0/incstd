#pragma once

#include <incstd/color/color_common.hpp>

namespace incom::standard::console::color_schemes {
using namespace incom::standard::color;

struct scheme16 {
    palette16 palette;
    inc_sRGB  foreground;
    inc_sRGB  backgrond;
    inc_sRGB  cursor;
    inc_sRGB  selection;
};
struct scheme256 {
    palette256 palette;
    inc_sRGB   foreground;
    inc_sRGB   backgrond;
    inc_sRGB   cursor;
    inc_sRGB   selection;
};

namespace windows_terminal {

inline constexpr scheme16 campbell{
    {{inc_sRGB{12, 12, 12}, inc_sRGB{197, 15, 31}, inc_sRGB{19, 161, 14}, inc_sRGB{193, 156, 0}, inc_sRGB{0, 55, 218},
      inc_sRGB{136, 23, 152}, inc_sRGB{58, 150, 221}, inc_sRGB{204, 204, 204}, inc_sRGB{118, 118, 118},
      inc_sRGB{231, 72, 86}, inc_sRGB{22, 198, 12}, inc_sRGB{249, 241, 165}, inc_sRGB{59, 120, 255},
      inc_sRGB{180, 0, 158}, inc_sRGB{97, 214, 214}, inc_sRGB{242, 242, 242}}},
    {204, 204, 204},
    {12, 12, 12},
    {255, 255, 255},
    {255, 255, 255}};

inline constexpr scheme16 dimidium{
    {{inc_sRGB{0, 0, 0}, inc_sRGB{204, 36, 29}, inc_sRGB{80, 204, 80}, inc_sRGB{204, 204, 29}, inc_sRGB{29, 80, 204},
      inc_sRGB{204, 29, 204}, inc_sRGB{29, 204, 204}, inc_sRGB{204, 204, 204}, inc_sRGB{102, 102, 102},
      inc_sRGB{204, 102, 102}, inc_sRGB{102, 204, 102}, inc_sRGB{204, 204, 102}, inc_sRGB{102, 102, 204},
      inc_sRGB{204, 102, 204}, inc_sRGB{102, 204, 204}, inc_sRGB{204, 204, 204}}},
    {204, 204, 204}, // foreground
    {0, 0, 0},       // background
    {204, 204, 204}, // cursor
    {204, 204, 204}  // selection
};

inline constexpr scheme16 dark_plus{
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
    {{inc_sRGB{40, 44, 52}, inc_sRGB{224, 108, 117}, inc_sRGB{152, 195, 121}, inc_sRGB{229, 192, 123},
      inc_sRGB{97, 175, 239}, inc_sRGB{198, 120, 221}, inc_sRGB{86, 182, 194}, inc_sRGB{220, 223, 228},
      inc_sRGB{92, 99, 112}, inc_sRGB{224, 108, 117}, inc_sRGB{152, 195, 121}, inc_sRGB{229, 192, 123},
      inc_sRGB{97, 175, 239}, inc_sRGB{198, 120, 221}, inc_sRGB{86, 182, 194}, inc_sRGB{220, 223, 228}}},
    {220, 223, 228},
    {40, 44, 52},
    {220, 223, 228},
    {220, 223, 228}};

inline constexpr scheme16 one_half_light{
    {{inc_sRGB{250, 250, 250}, inc_sRGB{224, 108, 117}, inc_sRGB{152, 195, 121}, inc_sRGB{184, 173, 104},
      inc_sRGB{97, 175, 239}, inc_sRGB{198, 120, 221}, inc_sRGB{86, 182, 194}, inc_sRGB{56, 58, 66},
      inc_sRGB{153, 153, 153}, inc_sRGB{224, 108, 117}, inc_sRGB{152, 195, 121}, inc_sRGB{184, 173, 104},
      inc_sRGB{97, 175, 239}, inc_sRGB{198, 120, 221}, inc_sRGB{86, 182, 194}, inc_sRGB{56, 58, 66}}},
    {56, 58, 66},
    {250, 250, 250},
    {56, 58, 66},
    {56, 58, 66}};

inline constexpr scheme16 solarized_dark{
    {{inc_sRGB{0, 43, 54}, inc_sRGB{220, 50, 47}, inc_sRGB{133, 153, 0}, inc_sRGB{181, 137, 0}, inc_sRGB{38, 139, 210},
      inc_sRGB{211, 54, 130}, inc_sRGB{42, 161, 152}, inc_sRGB{238, 232, 213}, inc_sRGB{88, 110, 117},
      inc_sRGB{203, 75, 22}, inc_sRGB{88, 110, 117}, inc_sRGB{101, 123, 131}, inc_sRGB{131, 148, 150},
      inc_sRGB{108, 113, 196}, inc_sRGB{147, 161, 161}, inc_sRGB{253, 246, 227}}},
    {131, 148, 150},
    {0, 43, 54},
    {131, 148, 150},
    {131, 148, 150}};

inline constexpr scheme16 solarized_light{
    {{inc_sRGB{253, 246, 227}, inc_sRGB{220, 50, 47}, inc_sRGB{133, 153, 0}, inc_sRGB{181, 137, 0},
      inc_sRGB{38, 139, 210}, inc_sRGB{211, 54, 130}, inc_sRGB{42, 161, 152}, inc_sRGB{7, 54, 66},
      inc_sRGB{238, 232, 213}, inc_sRGB{203, 75, 22}, inc_sRGB{88, 110, 117}, inc_sRGB{101, 123, 131},
      inc_sRGB{131, 148, 150}, inc_sRGB{108, 113, 196}, inc_sRGB{147, 161, 161}, inc_sRGB{0, 43, 54}}},
    {101, 123, 131},
    {253, 246, 227},
    {101, 123, 131},
    {101, 123, 131}};

inline constexpr scheme16 tango_dark{
    {{inc_sRGB{0, 0, 0}, inc_sRGB{204, 0, 0}, inc_sRGB{78, 154, 6}, inc_sRGB{196, 160, 0}, inc_sRGB{52, 101, 164},
      inc_sRGB{117, 80, 123}, inc_sRGB{6, 152, 154}, inc_sRGB{211, 215, 207}, inc_sRGB{85, 87, 83},
      inc_sRGB{239, 41, 41}, inc_sRGB{138, 226, 52}, inc_sRGB{252, 233, 79}, inc_sRGB{114, 159, 207},
      inc_sRGB{173, 127, 168}, inc_sRGB{52, 226, 226}, inc_sRGB{238, 238, 236}}},
    {238, 238, 238},
    {34, 34, 34},
    {238, 238, 238},
    {238, 238, 238}};

inline constexpr scheme16 tango_light{
    {{inc_sRGB{0, 0, 0}, inc_sRGB{204, 0, 0}, inc_sRGB{78, 154, 6}, inc_sRGB{196, 160, 0}, inc_sRGB{52, 101, 164},
      inc_sRGB{117, 80, 123}, inc_sRGB{6, 152, 154}, inc_sRGB{211, 215, 207}, inc_sRGB{85, 87, 83},
      inc_sRGB{239, 41, 41}, inc_sRGB{138, 226, 52}, inc_sRGB{252, 233, 79}, inc_sRGB{114, 159, 207},
      inc_sRGB{173, 127, 168}, inc_sRGB{52, 226, 226}, inc_sRGB{238, 238, 236}}},
    {56, 58, 66},
    {255, 255, 255},
    {56, 58, 66},
    {56, 58, 66}};

inline constexpr scheme16 cga{
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
    cga.palette,     // palette identical to CGA
    {170, 170, 170}, // foreground
    {0, 0, 0},       // background
    {170, 170, 170}, // cursor
    {85, 85, 85}     // selection (fallback)
};

inline constexpr scheme16 campbell_powershell{
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
} // namespace incom::standard::console::color_schemes
