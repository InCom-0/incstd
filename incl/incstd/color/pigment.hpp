//  MIT License
//
//  Copyright (c) 2025 Michal Lurie
//
//  Permission is hereby granted, free of charge, to any person obtaining a
//  copy of this software and associated documentation files (the "Software"),
//  to deal in the Software without restriction, including without limitation
//  the rights to use, copy, modify, merge, publish, distribute, sublicense,
//  and/or sell copies of the Software, and to permit persons to whom the
//  Software is furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in
//  all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//  DEALINGS IN THE SOFTWARE.


// pigment.hpp
#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <concepts>
#include <iomanip>
#include <numbers>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>


#include "color_common.hpp"


namespace incom::standard::color {
namespace detail {
inline constexpr int    SAMPLE_SIZE = 38;
inline constexpr double GAMMA       = 2.4;
inline constexpr double EPS_MIN     = 1e-12;
} // namespace detail

using arr_dbl38 = std::array<double, detail::SAMPLE_SIZE>;

enum class gamutMap_methods {
    clip = 1,
    map,
};

class Color;

namespace detail {
// Definition of constant spectral data and conversion matrices
// Contains arrays for various spectra (White, Cyan, Magenta, Yellow, Red, Green, Blue).
namespace BASE_SPECTRA {
inline constexpr arr_dbl38 W = {
    1.00116072718764,  1.00116065159728,  1.00116031922747,  1.00115867270789,  1.00115259844552,  1.00113252528998,
    1.00108500663327,  1.00099687889453,  1.00086525152274,  1.0006962900094,   1.00050496114888,  1.00030808187992,
    1.00011966602013,  0.999952765968407, 0.999821836899297, 0.999738609557593, 0.999709551639612, 0.999731930210627,
    0.999799436346195, 0.999900330316671, 1.00002040652611,  1.00014478793658,  1.00025997903412,  1.00035579697089,
    1.00042753780269,  1.00047623344888,  1.00050720967508,  1.00052519156373,  1.00053509606896,  1.00054022097482,
    1.00054272816784,  1.00054389569087,  1.00054448212151,  1.00054476959992,  1.00054489887762,  1.00054496254689,
    1.00054498927058,  1.000544996993};
inline constexpr arr_dbl38 C = {
    0.970585001322962,  0.970592498143425,  0.970625348729891,  0.970786806119017,  0.971368673228248,
    0.973163230621252,  0.976740223158765,  0.981587605491377,  0.986280265652949,  0.989949147689134,
    0.99249270153842,   0.994145680405256,  0.995183975033212,  0.995756750110818,  0.99591281828671,
    0.995606157834528,  0.994597600961854,  0.99221571549237,   0.986236452783249,  0.967943337264541,
    0.891285004244943,  0.536202477862053,  0.154108119001878,  0.0574575093228929, 0.0315349873107007,
    0.0222633920086335, 0.0182022841492439, 0.016299055973264,  0.0153656239334613, 0.0149111568733976,
    0.0146954339898235, 0.0145964146717719, 0.0145470156699655, 0.0145228771899495, 0.0145120341118965,
    0.0145066940939832, 0.0145044507314479, 0.0145038009464639};
inline constexpr arr_dbl38 M = {
    0.990673557319988, 0.990671524961979,  0.990662582353421,  0.990618107644795,  0.99045148087871,
    0.989871081400204, 0.98828660875964,   0.984290692797504,  0.973934905625306,  0.941817838460145,
    0.817390326195156, 0.432472805065729,  0.13845397825887,   0.0537347216940033, 0.0292174996673231,
    0.021313651750859, 0.0201349530181136, 0.0241323096280662, 0.0372236145223627, 0.0760506552706601,
    0.205375471942399, 0.541268903460439,  0.815841685086486,  0.912817704123976,  0.946339830166962,
    0.959927696331991, 0.966260595230312,  0.969325970058424,  0.970854536721399,  0.971605066528128,
    0.971962769757392, 0.972127272274509,  0.972209417745812,  0.972249577678424,  0.972267621998742,
    0.97227650946215,  0.972280243306874,  0.97228132482656};
inline constexpr arr_dbl38 Y = {
    0.0210523371789306, 0.0210564627517414, 0.0210746178695038, 0.0211649058448753, 0.0215027957272504,
    0.0226738799041561, 0.0258235649693629, 0.0334879385639851, 0.0519069663740307, 0.100749014833473,
    0.239129899706847,  0.534804312272748,  0.79780757864303,   0.911449894067384,  0.953797963004507,
    0.971241615465429,  0.979303123807588,  0.983380119507575,  0.985461246567755,  0.986435046976605,
    0.986738250670141,  0.986617882445032,  0.986277776758643,  0.985860592444056,  0.98547492767621,
    0.985176934765558,  0.984971574014181,  0.984846303415712,  0.984775351811199,  0.984738066625265,
    0.984719648311765,  0.984711023391939,  0.984706683300676,  0.984704554393091,  0.98470359630937,
    0.984703124077552,  0.98470292561509,   0.984702868122795};
inline constexpr arr_dbl38 R = {
    0.0315605737777207, 0.0315520718330149, 0.0315148215513658, 0.0313318044982702, 0.0306729857725527,
    0.0286480476989607, 0.0246450407045709, 0.0192960753663651, 0.0142066612220556, 0.0102942608878609,
    0.0076191460521811, 0.005898041083542,  0.0048233247781713, 0.0042298748350633, 0.0040599171299341,
    0.0043533695594676, 0.0053434425970201, 0.0076917201010463, 0.0135969795736536, 0.0316975442661115,
    0.107861196355249,  0.463812603168704,  0.847055405272011,  0.943185409393918,  0.968862150696558,
    0.978030667473603,  0.982043643854306,  0.983923623718707,  0.984845484154382,  0.985294275814596,
    0.985507295219825,  0.985605071539837,  0.985653849933578,  0.985677685033883,  0.985688391806122,
    0.985693664690031,  0.985695879848205,  0.985696521463762};
inline constexpr arr_dbl38 G = {
    0.0095560747554212, 0.0095581580120851, 0.0095673245444588, 0.0096129126297349, 0.0097837090401843,
    0.010378622705871,  0.0120026452378567, 0.0160977721473922, 0.026706190223168,  0.0595555440185881,
    0.186039826532826,  0.570579820116159,  0.861467768400292,  0.945879089767658,  0.970465486474305,
    0.97841363028445,   0.979589031411224,  0.975533536908632,  0.962288755397813,  0.92312157451312,
    0.793434018943111,  0.459270135902429,  0.185574103666303,  0.0881774959955372, 0.05436302287667,
    0.0406288447060719, 0.034221520431697,  0.0311185790956966, 0.0295708898336134, 0.0288108739348928,
    0.0284486271324597, 0.0282820301724731, 0.0281988376490237, 0.0281581655342037, 0.0281398910216386,
    0.0281308901665811, 0.0281271086805816, 0.0281260133612096};
inline constexpr arr_dbl38 B = {
    0.979404752502014,  0.97940070684313,   0.979382903470261,  0.979294364945594,  0.97896301460857,
    0.977814466694043,  0.974724321133836,  0.967198482343973,  0.949079657530575,  0.900850128940977,
    0.76315044546224,   0.465922171649319,  0.201263280451005,  0.0877524413419623, 0.0457176793291679,
    0.0284706050521843, 0.020527176756985,  0.0165302792310211, 0.0145135107212858, 0.0136003508637687,
    0.0133604258769571, 0.013548894314568,  0.0139594356366992, 0.014443425575357,  0.0148854440621406,
    0.0152254296999746, 0.0154592848180209, 0.0156018026485961, 0.0156824871281936, 0.0157248764360615,
    0.0157458108784121, 0.0157556123350225, 0.0157605443964911, 0.0157629637515278, 0.0157640525629106,
    0.015764589232951,  0.0157648147772649, 0.0157648801149616};
} // namespace BASE_SPECTRA

// detail::CIE Color Matching Functions weighted by D65 Standard Illuminant
namespace CIE {
inline constexpr std::array<arr_dbl38, 3> CMF = {
    {{0.0000646919989576, 0.0002194098998132, 0.0011205743509343, 0.0037666134117111, 0.011880553603799,
      0.0232864424191771, 0.0345594181969747, 0.0372237901162006, 0.0324183761091486, 0.021233205609381,
      0.0104909907685421, 0.0032958375797931, 0.0005070351633801, 0.0009486742057141, 0.0062737180998318,
      0.0168646241897775, 0.028689649025981,  0.0426748124691731, 0.0562547481311377, 0.0694703972677158,
      0.0830531516998291, 0.0861260963002257, 0.0904661376847769, 0.0850038650591277, 0.0709066691074488,
      0.0506288916373645, 0.035473961885264,  0.0214682102597065, 0.0125164567619117, 0.0068045816390165,
      0.0034645657946526, 0.0014976097506959, 0.000769700480928,  0.0004073680581315, 0.0001690104031614,
      0.0000952245150365, 0.0000490309872958, 0.0000199961492222},
     {0.000001844289444,  0.0000062053235865, 0.0000310096046799, 0.0001047483849269, 0.0003536405299538,
      0.0009514714056444, 0.0022822631748318, 0.004207329043473,  0.0066887983719014, 0.0098883960193565,
      0.0152494514496311, 0.0214183109449723, 0.0334229301575068, 0.0513100134918512, 0.070402083939949,
      0.0878387072603517, 0.0942490536184085, 0.0979566702718931, 0.0941521856862608, 0.0867810237486753,
      0.0788565338632013, 0.0635267026203555, 0.05374141675682,   0.042646064357412,  0.0316173492792708,
      0.020885205921391,  0.0138601101360152, 0.0081026402038399, 0.004630102258803,  0.0024913800051319,
      0.0012593033677378, 0.000541646522168,  0.0002779528920067, 0.0001471080673854, 0.0000610327472927,
      0.0000343873229523, 0.0000177059860053, 0.000007220974913},
     {0.000305017147638,
      0.0010368066663574,
      0.0053131363323992,
      0.0179543925899536,
      0.0570775815345485,
      0.113651618936287,
      0.17335872618355,
      0.196206575558657,
      0.186082370706296,
      0.139950475383207,
      0.0891745294268649,
      0.0478962113517075,
      0.0281456253957952,
      0.0161376622950514,
      0.0077591019215214,
      0.0042961483736618,
      0.0020055092122156,
      0.0008614711098802,
      0.0003690387177652,
      0.0001914287288574,
      0.0001495555858975,
      0.0000923109285104,
      0.0000681349182337,
      0.0000288263655696,
      0.0000157671820553,
      0.0000039406041027,
      0.000001584012587,
      0.0,
      0.0,
      0.0,
      0.0,
      0.0,
      0.0,
      0.0,
      0.0,
      0.0,
      0.0,
      0.0}}};
}

// Conversion matrices and constants for various color space transformations.
//
// see https://github.com/w3c/csswg-drafts/issues/5922}
// see https://github.com/color-js/color.js/blob/main/src/spaces/srgb-linear.js}
// see https://github.com/color-js/color.js/blob/main/src/spaces/oklab.js}
namespace CONVERSION {
inline constexpr std::array<inc_lRGB, 3> RGB_XYZ{{{0.41239079926595934, 0.357584339383878, 0.1804807884018343},
                                                  {0.21263900587151027, 0.715168678767756, 0.07219231536073371},
                                                  {0.01933081871559182, 0.11919477979462598, 0.9505321522496607}}};
inline constexpr std::array<inc_lRGB, 3> XYZ_RGB = {{{3.2409699419045226, -1.537383177570094, -0.4986107602930034},
                                                     {-0.9692436362808796, 1.8759675015077202, 0.04155505740717559},
                                                     {0.05563007969699366, -0.20397695888897652, 1.0569715142428786}}};
inline constexpr std::array<inc_lRGB, 3> XYZ_LMS = {{{0.819022437996703, 0.3619062600528904, -0.1288737815209879},
                                                     {0.0329836539323885, 0.9292868615863434, 0.0361446663506424},
                                                     {0.0481771893596242, 0.2642395317527308, 0.6335478284694309}}};
inline constexpr std::array<inc_lRGB, 3> LMS_XYZ = {{{1.2268798758459243, -0.5578149944602171, 0.2813910456659647},
                                                     {-0.0405757452148008, 1.112286803280317, -0.0717110580655164},
                                                     {-0.0763729366746601, -0.4214933324022432, 1.5869240198367816}}};
inline constexpr std::array<inc_lRGB, 3> LMS_LAB = {{{0.210454268309314, 0.7936177747023054, -0.0040720430116193},
                                                     {1.9779985324311684, -2.4285922420485799, 0.450593709617411},
                                                     {0.0259040424655478, 0.7827717124575296, -0.8086757549230774}}};
inline constexpr std::array<inc_lRGB, 3> LAB_LMS = {{{1.0, 0.3963377773761749, 0.2158037573099136},
                                                     {1.0, -0.1055613458156586, -0.0638541728258133},
                                                     {1.0, -0.0894841775298119, -1.2914855480194092}}};
} // namespace CONVERSION


// CONCEPTS AND TYPE TRAITS
// Primary template: defaults to false
template <typename T>
struct is_std_array : std::false_type {};

// Partial specialization for std::array
template <typename T, std::size_t N>
struct is_std_array<std::array<T, N>> : std::true_type {};

// Helper variable template (C++14+)
template <typename T>
inline constexpr bool is_std_array_v = is_std_array<T>::value;

template <typename M, typename V>
concept _is_matrix_and_range =
    std::ranges::range<M> && std::ranges::range<std::ranges::range_value_t<M>> && std::ranges::range<V>;

template <typename M, typename V>
concept _is_matrixArray_and_range = _is_matrix_and_range<M, V> && is_std_array_v<M>;

// COMPUTATIONAL HELPER FUNCTIONS
template <typename M, typename V>
requires _is_matrixArray_and_range<M, V> && is_std_array_v<V>
inline constexpr auto mulMatVec(M const &matrix, V const &vec) -> std::array<double, std::tuple_size<M>{}> {
    std::array<double, std::tuple_size<M>{}> res;
    for (std::size_t i = 0; i < std::tuple_size<M>{}; ++i) {
        res[i] = 0.0;
        for (std::size_t j = 0; j < std::tuple_size<V>{}; ++j) { res[i] += matrix[i][j] * vec[j]; }
    }
    return res;
}

template <typename M, typename V>
requires _is_matrixArray_and_range<M, V>
inline constexpr auto mulMatVec(M const &matrix, V const &vec) -> std::array<double, std::tuple_size<M>{}> {
    std::size_t const                        cols = vec.size();
    std::array<double, std::tuple_size<M>{}> res;
    for (std::size_t i = 0; i < std::tuple_size<M>{}; ++i) {
        res[i] = 0.0;
        for (std::size_t j = 0; j < cols; ++j) { res[i] += matrix[i][j] * vec[j]; }
    }
    return res;
}

template <typename M, typename V>
requires _is_matrix_and_range<M, V>
inline constexpr auto mulMatVec(M const &matrix, V const &vec) {
    std::size_t const   rows = matrix.size();
    std::size_t const   cols = vec.size();
    std::vector<double> res(rows, 0.0);
    for (std::size_t i = 0; i < rows; ++i) {
        for (std::size_t j = 0; j < cols; ++j) { res[i] += matrix[i][j] * vec[j]; }
    }
    return res;
}

inline constexpr auto mulMatVec(std::array<inc_lRGB, 3> const &matrix, inc_lRGB const &vec) {
    inc_lRGB res{0, 0, 0};
    res.r += matrix[0].r * vec.r;
    res.r += matrix[0].g * vec.g;
    res.r += matrix[0].b * vec.b;
    res.g += matrix[1].r * vec.r;
    res.g += matrix[1].g * vec.g;
    res.g += matrix[1].b * vec.b;
    res.b += matrix[2].r * vec.r;
    res.b += matrix[2].g * vec.g;
    res.b += matrix[2].b * vec.b;
    return res;
}

inline constexpr double uncompand(double x) {
    return x > 0.04045 ? std::pow((x + 0.055) / 1.055, detail::GAMMA) : x / 12.92;
}

inline constexpr double compand(double x) {
    return x > 0.0031308 ? 1.055 * std::pow(x, 1.0 / detail::GAMMA) - 0.055 : x * 12.92;
}

inline constexpr bool inGamut(const inc_lRGB &lRGB, double epsilon = 0.0) {
    if (lRGB.r < -epsilon || lRGB.r > 1.0 + epsilon) { return false; }
    if (lRGB.g < -epsilon || lRGB.g > 1.0 + epsilon) { return false; }
    if (lRGB.b < -epsilon || lRGB.b > 1.0 + epsilon) { return false; }

    return true;
}

inline double deltaEOK(const inc_lRGB &OK1, const inc_lRGB &OK2) {
    double d0 = OK1.r - OK2.r;
    double d1 = OK1.g - OK2.g;
    double d2 = OK1.b - OK2.b;
    return std::sqrt(d0 * d0 + d1 * d1 + d2 * d2);
}

// Computes the Kubelka–Munk absorption/scattering parameter KS for a given spectral reflectance R.
//
// In Kubelka–Munk theory, the KS function reflects the ratio that controls the conversion from spectral
// reflectance to an equivalent absorption/scattering coeffidetail::CIEnt. The formulation
// <code>(1 - R)² / (2 * R)</code> is a common approximation that assumes a diffusely scattering medium.
inline constexpr double KS_func(double R) {
    return (1.0 - R) * (1.0 - R) / (2.0 * R);
}

// Computes the Kubelka–Munk mixing coeffidetail::CIEnt KM from a given KS value.
//
// The KM function transforms the KS parameter into a measure that can be linearly mixed.
// This conversion is essential because the Kubelka–Munk model assumes that when pigments are
// mixed, the resulting reflectance is a function of the weighted combination of the pigment
// absorption and scattering properties. The formula used here:
//
// <pre>
// KM(KS) = 1 + KS - √(KS² + 2KS)
// </pre>
//
// provides the appropriate transformation for blending multiple pigment spectra.
inline double KM_func(double KS) {
    return 1.0 + KS - std::sqrt(std::pow(KS, 2) + (2.0 * KS));
}

constexpr Color pigment_blend_HLPR(std::vector<double> const &factors, std::vector<inc_lRGB> const &lRGBs,
                                   std::vector<double> const &luminances);

} // namespace detail


// #######################################
// ### MAIN LIBRARY INTERFACE PART    ####
// #######################################

// Converter is a uninstantiable 'static utility' class for converting various color formats between each other
class Converter {
public:
    // All the following should be made constexpr once MSVC gets up to speed
    static inc_lRGB sRGB_to_lRGB(const inc_sRGB &sRGB);
    static inc_sRGB lRGB_to_sRGB(const inc_lRGB &lRGB);
    static inc_lRGB XYZ_to_lRGB(const inc_lRGB &XYZ);
    static inc_lRGB lRGB_to_XYZ(const inc_lRGB &lRGB);
    static inc_lRGB XYZ_to_OKLab(const inc_lRGB &XYZ);
    static inc_lRGB OKLab_to_XYZ(const inc_lRGB &OKLab);
    static inc_lRGB OKLab_to_OKLCh(const inc_lRGB &OKLab);
    static inc_lRGB OKLCh_to_OKLab(const inc_lRGB &OKLCh);

    static constexpr arr_dbl38          parseSpectralReflectanceFromLRGB(const inc_lRGB &lRGB);
    static constexpr inc_sRGB parseCssColor(const std::string &str);

    static constexpr double luminance_from_lRGB(const inc_lRGB &lRBG) {
        // Only the Y from the XYZ tupple needs to be computed for luminance
        double Y  = 0;
        Y        += detail::CONVERSION::RGB_XYZ[1].r * lRBG.r;
        Y        += detail::CONVERSION::RGB_XYZ[1].g * lRBG.g;
        Y        += detail::CONVERSION::RGB_XYZ[1].b * lRBG.b;
        return std::max(Y, detail::EPS_MIN);
    }

    static constexpr double luminance_from_sRGB(const inc_sRGB &sRBG) {
        return luminance_from_lRGB(sRGB_to_lRGB(sRBG));
    }

    static constexpr arr_dbl38 KS_from_lRGB(const inc_lRGB &lRBG) {
        // Variable used as both temp Reflectance storage AND return value (reflectance gets overwritten)
        arr_dbl38 R_and_KS_res = parseSpectralReflectanceFromLRGB(lRBG);
        for (int i = 0; i < detail::SAMPLE_SIZE; ++i) { R_and_KS_res[i] = detail::KS_func(R_and_KS_res[i]); }
        return R_and_KS_res;
    }


    Converter()                             = delete; // no default constructor
    Converter(const Converter &)            = delete; // no copy
    Converter &operator=(const Converter &) = delete; // no assignment
    ~Converter()                            = delete; // no destructor
    void *operator new(std::size_t)         = delete; // no heap
    void *operator new[](std::size_t)       = delete; // no heap array
    void *operator new(std::size_t, void *) = delete; // blocked placement-new
};

// #######################################
// ### 'FAT' COLOR CLASS      ######
// #######################################
class Color {
public:
    // Public data
    inc_sRGB  sRGB; // 0..255 (no alpha, ignoring alpha)
    inc_lRGB  lRGB; // linear RGB [0..1]
    inc_lRGB  XYZ;  // XYZ space
    arr_dbl38 R;    // spectral reflectance, size = SIZE

    // Tinting strength (default 1)
    double tintingStrength = 1.0;
    double luminance       = 0.0;

private:
    // Lazy-computed caches
    mutable std::optional<inc_lRGB>  _OKLab;
    mutable std::optional<inc_lRGB>  _OKLCh;
    mutable std::optional<arr_dbl38> _KS;

public:
    // Constructors
    constexpr Color() {
        sRGB      = {0, 0, 0};
        lRGB      = {0.0, 0.0, 0.0};
        R         = Converter::parseSpectralReflectanceFromLRGB(lRGB);
        XYZ       = Converter::lRGB_to_XYZ(lRGB);
        luminance = std::max(XYZ.g, detail::EPS_MIN);
    }
    constexpr Color(const std::string &css) {
        auto c    = Converter::parseCssColor(css);
        sRGB      = c;
        lRGB      = Converter::sRGB_to_lRGB(sRGB);
        R         = Converter::parseSpectralReflectanceFromLRGB(lRGB);
        XYZ       = Converter::lRGB_to_XYZ(lRGB);
        luminance = std::max(XYZ.g, detail::EPS_MIN);
    }
    constexpr Color(const inc_sRGB &srgb_) {
        sRGB      = srgb_;
        lRGB      = Converter::sRGB_to_lRGB(sRGB);
        R         = Converter::parseSpectralReflectanceFromLRGB(lRGB);
        XYZ       = Converter::lRGB_to_XYZ(lRGB);
        luminance = std::max(XYZ.g, detail::EPS_MIN);
    }
    constexpr Color(const inc_lRGB &lrgb_) {
        sRGB      = Converter::lRGB_to_sRGB(lrgb_);
        lRGB      = lrgb_;
        R         = Converter::parseSpectralReflectanceFromLRGB(lRGB);
        XYZ       = Converter::lRGB_to_XYZ(lRGB);
        luminance = std::max(XYZ.g, detail::EPS_MIN);
    }
    constexpr Color(const std::vector<double> &spectralR) {
        if ((int)spectralR.size() != detail::SAMPLE_SIZE) {
            throw std::invalid_argument("spectralR must have length SIZE");
        }
        for (size_t i = 0; auto &r_item : R) { r_item = spectralR[i++]; }
        XYZ       = detail::mulMatVec(detail::CIE::CMF, R);
        lRGB      = Converter::XYZ_to_lRGB(XYZ);
        sRGB      = Converter::lRGB_to_sRGB(lRGB);
        luminance = std::max(XYZ.g, detail::EPS_MIN);
    }
    constexpr Color(const arr_dbl38 &spectralR) {
        R         = spectralR;
        XYZ       = detail::mulMatVec(detail::CIE::CMF, R);
        lRGB      = Converter::XYZ_to_lRGB(XYZ);
        sRGB      = Converter::lRGB_to_sRGB(lRGB);
        luminance = std::max(XYZ.g, detail::EPS_MIN);
    }

    // Getters
    constexpr inc_lRGB const &OKLab() const {
        if (! _OKLab) { _OKLab = Converter::XYZ_to_OKLab(XYZ); }
        return *_OKLab;
    }
    constexpr inc_lRGB const &OKLCh() const {
        if (! _OKLCh) { _OKLCh = Converter::OKLab_to_OKLCh(OKLab()); }
        return *_OKLCh;
    }
    constexpr arr_dbl38 const &KS() const {
        if (! _KS) {
            _KS           = arr_dbl38{};
            auto &_KS_ref = _KS.value();
            for (int i = 0; i < detail::SAMPLE_SIZE; ++i) { _KS_ref[i] = detail::KS_func(R[i]); }
        }
        return _KS.value();
    }

    // Methods
private:
    // gamutMap (binary search of chroma)
    static constexpr Color gamutMap(const Color &color, double jnd = 0.03, double eps = 0.0001);

public:
    constexpr Color       toGamut(const gamutMap_methods &method = gamutMap_methods::map) const;
    constexpr std::string toString(const std::string      &format = "hex",
                                   const gamutMap_methods &method = gamutMap_methods::map) const;
};


// #######################################
// ### 'PIGMENT' COLOR MIXING      ######
// #######################################
//
// Suitable when it is desirable to imitate mixing colours 'as-if' they were real world pigments
// Based on Kubelka-Munk theory (https://en.wikipedia.org/wiki/Kubelka%E2%80%93Munk_theory)
// https://en.wikipedia.org/wiki/Pigment
namespace pigment {

// Blend mixes multiple colors using a model based on the Kubelka–Munk theory.
//
// This function implements a blending algorithm that is inspired by the Kubelka–Munk theory,
// which models how light interacts with diffusely scattering and absorbing layers (such as pigments or paints).
// The approach is as follows:
//
// - For each wavelength band (with SIZE samples), compute a weighted average of the KS values.
// - Weights are determined by the square of a factor that considers both the square-root of the color's
//   luminance and its tinting strength multiplied by a user-specified factor.
// - The resulting weighted KS average is then converted back using the KM function to obtain the
//   mixed spectral reflectance.
//
// In effect, this method blends pigments based on their optical absorption and scattering properties,
// providing a physically motivated approximation for pigment blending as described by Kubelka–Munk.
//
// Each argument should be provided as an array of two elements: [Color, factor]. The factor determines
// the influence of that particular color in the overall mix (simple weight)

// Color class blending
constexpr Color blend(const std::vector<std::pair<Color, double>> &colors);
constexpr Color blend(const std::vector<Color> &colors);

template <typename... PR_Cs_F>
requires(sizeof...(PR_Cs_F) > 1) && (std::same_as<std::remove_cvref_t<PR_Cs_F>, std::pair<Color, double>> && ...)
constexpr Color blend(const PR_Cs_F &...colors);

template <typename... Cs>
requires(sizeof...(Cs) > 1) && (std::same_as<std::remove_cvref_t<Cs>, Color> && ...)
constexpr Color blend(const Cs &...colors);


// L_RGB blending
constexpr Color blend(const std::vector<std::pair<inc_lRGB, double>> &colors_lrgb_f);
constexpr Color blend(const std::vector<inc_lRGB> &colors_lrgb_f);

template <typename... PR_LRGBs_F>
requires(sizeof...(PR_LRGBs_F) > 1) &&
        (std::same_as<std::remove_cvref_t<PR_LRGBs_F>, std::pair<inc_lRGB, double>> && ...)
constexpr Color blend(const PR_LRGBs_F &...colors);

template <typename... LRGBs>
requires(sizeof...(LRGBs) > 1) && (std::same_as<std::remove_cvref_t<LRGBs>, inc_lRGB> && ...)
constexpr Color blend(const LRGBs &...colors);


// S_RGB blending
constexpr Color blend(const std::vector<std::pair<inc_sRGB, double>> &colors_srgb_f);
constexpr Color blend(const std::vector<inc_sRGB> &colors_srgb);

template <typename... PR_SRGBs_F>
requires(sizeof...(PR_SRGBs_F) > 1) &&
        (std::same_as<std::remove_cvref_t<PR_SRGBs_F>, std::pair<inc_sRGB, double>> && ...)
constexpr Color blend(const PR_SRGBs_F &...colors);

template <typename... SRGBs>
requires(sizeof...(SRGBs) > 1) && (std::same_as<std::remove_cvref_t<SRGBs>, inc_sRGB> && ...)
constexpr Color blend(const SRGBs &...colors);


// Generates a palette of colors transitioning between two colors.
constexpr std::vector<Color> palette(Color const &a, Color const &b, size_t const sz);

// Interpolates between multiple colors based on a parameter t.
// Each additional argument should be an array with two elements: [Color, position].
constexpr Color gradient(double t, const std::vector<std::pair<Color, double>> &stops);
} // namespace pigment

// ###############################
// ### IMPLEMENTATION PART    ####
// ###############################

inline inc_lRGB Converter::sRGB_to_lRGB(const inc_sRGB &sRGB) {
    return {detail::uncompand(sRGB.r / 255.0), detail::uncompand(sRGB.g / 255.0), detail::uncompand(sRGB.b / 255.0)};
}
inline inc_sRGB Converter::lRGB_to_sRGB(const inc_lRGB &lRGB) {
    return {std::uint8_t(std::round(detail::compand(lRGB.r) * 255.0)),
            std::uint8_t(std::round(detail::compand(lRGB.g) * 255.0)),
            std::uint8_t(std::round(detail::compand(lRGB.b) * 255.0))};
}
inline inc_lRGB Converter::XYZ_to_lRGB(const inc_lRGB &XYZ) {
    return detail::mulMatVec(detail::CONVERSION::XYZ_RGB, XYZ);
}
inline inc_lRGB Converter::lRGB_to_XYZ(const inc_lRGB &lRGB) {
    return detail::mulMatVec(detail::CONVERSION::RGB_XYZ, lRGB);
}
inline inc_lRGB Converter::XYZ_to_OKLab(const inc_lRGB &XYZ) {
    // lms = mat * XYZ
    inc_lRGB lms = detail::mulMatVec(detail::CONVERSION::XYZ_LMS, XYZ);
    lms.r        = std::cbrt(lms.r);
    lms.g        = std::cbrt(lms.g);
    lms.b        = std::cbrt(lms.b);
    inc_lRGB lab = detail::mulMatVec(detail::CONVERSION::LMS_LAB, lms);
    return lab;
}
inline inc_lRGB Converter::OKLab_to_XYZ(const inc_lRGB &OKLab) {
    inc_lRGB lms = detail::mulMatVec(detail::CONVERSION::LAB_LMS, OKLab);
    lms.r        = std::pow(lms.r, 3);
    lms.g        = std::pow(lms.g, 3);
    lms.b        = std::pow(lms.b, 3);
    inc_lRGB XYZ = detail::mulMatVec(detail::CONVERSION::LMS_XYZ, lms);
    return XYZ;
}
inline inc_lRGB Converter::OKLab_to_OKLCh(const inc_lRGB &OKLab) {
    double h = std::atan2(OKLab.b, OKLab.g) * 180.0 / std::numbers::pi;
    if (h < 0.0) { h += 360.0; }
    return {OKLab.r, std::sqrt(std::pow(OKLab.g, 2) + std::pow(OKLab.b, 2)), h};
}
inline inc_lRGB Converter::OKLCh_to_OKLab(const inc_lRGB &OKLCh) {
    return {OKLCh.r, OKLCh.g * std::cos(OKLCh.b * std::numbers::pi / 180.0),
            OKLCh.g * std::sin(OKLCh.b * std::numbers::pi / 180.0)};
}


constexpr arr_dbl38 Converter::parseSpectralReflectanceFromLRGB(const inc_lRGB &lRGB) {
    // This is the inverse of lRGB_to_R
    // The code subtracts minimum w, c, m, y, r, g, b contributions using detail::BASE_SPECTRA.

    double const w   = std::min(std::min(lRGB.r, lRGB.g), lRGB.b);
    inc_lRGB     cpy = {lRGB.r - w, lRGB.g - w, lRGB.b - w};

    double const c = std::min(cpy.g, cpy.b);
    double const m = std::min(cpy.r, cpy.b);
    double const y = std::min(cpy.r, cpy.g);
    double const r = std::max(0.0, std::min(cpy.r - cpy.b, cpy.r - cpy.g));
    double const g = std::max(0.0, std::min(cpy.g - cpy.b, cpy.g - cpy.r));
    double const b = std::max(0.0, std::min(cpy.b - cpy.g, cpy.b - cpy.r));

    arr_dbl38 R{};
    for (int i = 0; i < detail::SAMPLE_SIZE; ++i) {
        // ensure at least epsilon
        R[i] = std::max(detail::EPS_MIN, w * detail::BASE_SPECTRA::W[i] + c * detail::BASE_SPECTRA::C[i] +
                                             m * detail::BASE_SPECTRA::M[i] + y * detail::BASE_SPECTRA::Y[i] +
                                             r * detail::BASE_SPECTRA::R[i] + g * detail::BASE_SPECTRA::G[i] +
                                             b * detail::BASE_SPECTRA::B[i]);
    }
    return R;
}

// CSS color parsing (#hex or rgb(...))
constexpr inc_sRGB Converter::parseCssColor(const std::string &str) {
    if (str.empty()) { throw std::invalid_argument("Empty CSS color string"); }
    if (str[0] == '#') {
        std::string hex = str.substr(1);
        if (hex.size() == 3) {
            // e.g. "#abc" → "aabbcc"
            std::string h2;
            h2.reserve(6);
            for (char c : hex) {
                h2.push_back(c);
                h2.push_back(c);
            }
            hex = h2;
        }
        if (hex.size() != 6 && hex.size() != 8) { throw std::invalid_argument("Invalid hex color length"); }
        std::uint8_t r = std::stoi(hex.substr(0, 2), nullptr, 16);
        std::uint8_t g = std::stoi(hex.substr(2, 2), nullptr, 16);
        std::uint8_t b = std::stoi(hex.substr(4, 2), nullptr, 16);
        int          a = 255;
        if (hex.size() == 8) { a = std::stoi(hex.substr(6, 2), nullptr, 16); }
        return {r, g, b};
    }
    else if (str.rfind("rgb", 0) == 0) {
        auto open  = str.find('(');
        auto close = str.find(')');
        if (open == std::string::npos || close == std::string::npos) {
            throw std::invalid_argument("Malformed rgb() string");
        }
        std::string              inner = str.substr(open + 1, close - open - 1);
        std::vector<std::string> parts;
        {
            std::istringstream iss(inner);
            std::string        part;
            while (std::getline(iss, part, ',')) { parts.push_back(part); }
        }
        if (parts.size() < 3) { throw std::invalid_argument("rgb() must have 3 components"); }
        inc_sRGB res{0, 0, 0};

        auto hlpr = [&, i = 0](std::uint8_t &ref) mutable {
            std::string s = parts[i];
            // trim whitespace
            while (! s.empty() && std::isspace((unsigned char)s.front())) { s.erase(s.begin()); }
            while (! s.empty() && std::isspace((unsigned char)s.back())) { s.pop_back(); }
            bool   isPercent = (s.back() == '%');
            double val       = std::stod(s);
            if (isPercent && i < 3) { ref = int(std::round(val * 2.55)); }
            else { ref = int(std::round(val)); }
            i++;
        };

        hlpr(res.r);
        hlpr(res.g);
        hlpr(res.b);

        return res;
    }
    else { throw std::invalid_argument("Unsupported CSS color format: " + str); }
}

// gamutMap (binary search of chroma)
constexpr Color Color::gamutMap(const Color &color, double jnd, double eps) {
    double L = color.OKLCh().r;
    if (L >= 1.0) { return Color(inc_sRGB{255, 255, 255}); }
    if (L <= 0.0) { return Color(inc_sRGB{0, 0, 0}); }
    if (detail::inGamut(color.lRGB)) { return color; }

    double h    = color.OKLCh().b;
    double lo   = 0.0;
    double hi   = color.OKLCh().g;
    bool   loIn = true;

    // clipped = lRGB_to_OKLab( clamp each component )
    inc_lRGB clippedOK = Converter::XYZ_to_OKLab(Converter::lRGB_to_XYZ(
        {std::clamp(color.lRGB.r, 0.0, 1.0), std::clamp(color.lRGB.g, 0.0, 1.0), std::clamp(color.lRGB.b, 0.0, 1.0)}));
    double   E         = detail::deltaEOK(clippedOK, Converter::XYZ_to_OKLab(Converter::lRGB_to_XYZ(color.lRGB)));
    if (E < jnd) {
        // short-circuit
        inc_lRGB tmpXYZ  = Converter::OKLab_to_XYZ(clippedOK);
        inc_lRGB tmplRGB = Converter::XYZ_to_lRGB(tmpXYZ);
        return Color(tmplRGB);
    }

    while (hi - lo > eps) {
        double mid = 0.5 * (lo + hi);
        auto   ok  = Converter::OKLCh_to_OKLab({L, mid, h});
        auto   XYZ = Converter::OKLab_to_XYZ(ok);
        auto   lr  = Converter::XYZ_to_lRGB(XYZ);

        if (loIn && detail::inGamut(lr)) { lo = mid; }
        else {
            auto clipped2OK = Converter::XYZ_to_OKLab(
                Converter::lRGB_to_XYZ({std::clamp(color.lRGB.r, 0.0, 1.0), std::clamp(color.lRGB.g, 0.0, 1.0),
                                        std::clamp(color.lRGB.b, 0.0, 1.0)}));
            double dist = detail::deltaEOK(clipped2OK, ok);
            if (dist < jnd) {
                if (jnd - dist < eps) { break; }
                else {
                    loIn = false;
                    lo   = mid;
                }
            }
            else { hi = mid; }
        }
    }

    auto ok  = Converter::OKLCh_to_OKLab({L, lo, h});
    auto xyz = Converter::OKLab_to_XYZ(ok);
    auto lr  = Converter::XYZ_to_lRGB(xyz);
    return Color(lr);
}

constexpr Color Color::toGamut(const gamutMap_methods &method) const {
    if (method == gamutMap_methods::clip) {
        inc_sRGB clipped = sRGB;
        return Color(clipped);
    }
    else if (method == gamutMap_methods::clip) { return gamutMap(*this); }
    else { throw std::invalid_argument("Unknown gamut mapping method: "); }
}

constexpr std::string Color::toString(const std::string &format, const gamutMap_methods &method) const {
    inc_sRGB outRGB;
    if (! detail::inGamut(lRGB)) {
        if (method == gamutMap_methods::clip) {
            // for (int i = 0; i < 3; ++i) { outRGB[i] = int(std::round(std::clamp(sRGB[i], 0, 255))); }
            outRGB = sRGB;
        }
        else if (method == gamutMap_methods::map) {
            Color mapped = gamutMap(*this);
            outRGB       = {mapped.sRGB.r, mapped.sRGB.g, mapped.sRGB.b};
        }
        else { throw std::invalid_argument("Unknown method in toString: "); }
    }
    else { outRGB = {sRGB.r, sRGB.g, sRGB.b}; }

    if (format == "hex" || format == "HEX" || format == "Hex") {
        std::ostringstream oss;
        oss << "#";
        oss << std::uppercase << std::hex << std::setw(2) << std::setfill('0') << (outRGB.r & 0xFF);
        oss << std::uppercase << std::hex << std::setw(2) << std::setfill('0') << (outRGB.b & 0xFF);
        oss << std::uppercase << std::hex << std::setw(2) << std::setfill('0') << (outRGB.g & 0xFF);
        return oss.str();
    }
    else if (format == "rgb" || format == "RGB") {
        std::ostringstream oss;
        oss << "rgb(" << outRGB.r << "," << outRGB.g << "," << outRGB.b << ")";
        return oss.str();
    }
    else { throw std::invalid_argument("Unknown format in toString: " + format); }
}

// ##########################################
// ### 'PIGMENT' COLOR MIXING IMPL     ######
// ##########################################

// Color class blending
inline constexpr Color pigment::blend(const std::vector<std::pair<Color, double>> &colors) {
    std::vector<arr_dbl38> KSs;
    KSs.reserve(colors.size());
    for (auto const &col : colors) { KSs.push_back(Converter::KS_from_lRGB(col.first.lRGB)); }

    arr_dbl38 Rm{};
    for (int i = 0; i < detail::SAMPLE_SIZE; ++i) {
        double ksSum   = 0.0;
        double concSum = 0.0;
        for (size_t j = 0; auto const &col : colors) {
            // Factor is always '1' is this case so noop for multiplication
            double const conc  = std::pow(col.second, 2) * std::pow(col.first.tintingStrength, 2) * col.first.luminance;
            concSum           += conc;
            ksSum             += KSs[j][i] * conc;
        }
        Rm[i] = detail::KM_func(ksSum / concSum);
    }
    Color cc(Rm);
    return cc.lRGB;
}

inline constexpr Color pigment::blend(const std::vector<Color> &colors) {
    std::vector<arr_dbl38> KSs;
    KSs.reserve(colors.size());
    for (auto const &col : colors) { KSs.push_back(Converter::KS_from_lRGB(col.lRGB)); }

    arr_dbl38 Rm{};
    for (int i = 0; i < detail::SAMPLE_SIZE; ++i) {
        double ksSum   = 0.0;
        double concSum = 0.0;
        for (size_t j = 0; auto const &col : colors) {
            // Factor is always '1' is this case so noop for multiplication
            double const conc  = std::pow(col.tintingStrength, 2) * col.luminance;
            concSum           += conc;
            ksSum             += KSs[j][i] * conc;
        }
        Rm[i] = detail::KM_func(ksSum / concSum);
    }
    Color cc(Rm);
    return cc.lRGB;
}

template <typename... PR_Cs_F>
requires(sizeof...(PR_Cs_F) > 1) && (std::same_as<std::remove_cvref_t<PR_Cs_F>, std::pair<Color, double>> && ...)
constexpr Color pigment::blend(const PR_Cs_F &...colors) {

    std::vector<double>   factors;
    std::vector<inc_lRGB> lRGBs;
    std::vector<double>   luminances;
    factors.reserve(sizeof...(PR_Cs_F));
    lRGBs.reserve(sizeof...(PR_Cs_F));
    luminances.reserve(sizeof...(PR_Cs_F));

    // Pack expansion to push all we need from the pack inside the vectors
    ((lRGBs.push_back(colors.first.lRGB), luminances.push_back(colors.first.luminance),
      factors.push_back(colors.second)),
     ...);

    return detail::pigment_blend_HLPR(factors, lRGBs, luminances);
}

template <typename... Cs>
requires(sizeof...(Cs) > 1) && (std::same_as<std::remove_cvref_t<Cs>, Color> && ...)
constexpr Color pigment::blend(const Cs &...colors) {
    std::vector<double>   factors(sizeof...(Cs), 1);
    std::vector<inc_lRGB> lRGBs;
    std::vector<double>   luminances;
    lRGBs.reserve(sizeof...(Cs));
    luminances.reserve(sizeof...(Cs));

    // Pack expansion to push all we need from the pack inside the vectors
    ((lRGBs.push_back(colors.first.lRGB), luminances.push_back(colors.first.luminance)), ...);

    return detail::pigment_blend_HLPR(factors, lRGBs, luminances);
}


// L_RGB blending
inline constexpr Color pigment::blend(const std::vector<std::pair<inc_lRGB, double>> &colors_lrgb_f) {

    std::vector<double>   factors;
    std::vector<inc_lRGB> lRGBs;
    std::vector<double>   luminances;
    factors.reserve(colors_lrgb_f.size());
    lRGBs.reserve(colors_lrgb_f.size());
    luminances.reserve(colors_lrgb_f.size());

    for (size_t i = 0; i < colors_lrgb_f.size(); ++i) {
        lRGBs.push_back(colors_lrgb_f[i].first);
        luminances.push_back(Converter::luminance_from_lRGB(colors_lrgb_f[i].first));
        factors.push_back(colors_lrgb_f[i].second);
    }

    return detail::pigment_blend_HLPR(factors, lRGBs, luminances);
}

inline constexpr Color pigment::blend(const std::vector<inc_lRGB> &colors_lrgb_f) {
    std::vector<double> factors(colors_lrgb_f.size(), 1);
    std::vector<double> luminances;
    luminances.reserve(colors_lrgb_f.size());

    for (size_t i = 0; i < colors_lrgb_f.size(); ++i) {
        luminances.push_back(Converter::luminance_from_lRGB(colors_lrgb_f[i]));
    }

    return detail::pigment_blend_HLPR(factors, colors_lrgb_f, luminances);
}

template <typename... PR_LRGBs_F>
requires(sizeof...(PR_LRGBs_F) > 1) &&
        (std::same_as<std::remove_cvref_t<PR_LRGBs_F>, std::pair<inc_lRGB, double>> && ...)
constexpr Color pigment::blend(const PR_LRGBs_F &...colors) {
    std::vector<double>   factors;
    std::vector<inc_lRGB> lRGBs;
    std::vector<double>   luminances;
    factors.reserve(sizeof...(PR_LRGBs_F));
    lRGBs.reserve(sizeof...(PR_LRGBs_F));
    luminances.reserve(sizeof...(PR_LRGBs_F));

    // Pack expansion to push all we need from the pack inside the vectors
    ((lRGBs.push_back(colors.first), luminances.push_back(Converter::luminance_from_lRGB(colors.first)),
      factors.push_back(colors.second)),
     ...);

    return detail::pigment_blend_HLPR(factors, lRGBs, luminances);
}

template <typename... LRGBs>
requires(sizeof...(LRGBs) > 1) && (std::same_as<std::remove_cvref_t<LRGBs>, inc_lRGB> && ...)
constexpr Color pigment::blend(const LRGBs &...colors) {
    std::vector<double>   factors(sizeof...(LRGBs), 1.0);
    std::vector<inc_lRGB> lRGBs;
    std::vector<double>   luminances;
    lRGBs.reserve(sizeof...(LRGBs));
    luminances.reserve(sizeof...(LRGBs));

    // Pack expansion to push all we need from the pack inside the vectors
    ((lRGBs.push_back(colors.first), luminances.push_back(Converter::luminance_from_lRGB(colors.first))), ...);

    return detail::pigment_blend_HLPR(factors, lRGBs, luminances);
}


// S_RGB blending
inline constexpr Color pigment::blend(const std::vector<std::pair<inc_sRGB, double>> &colors_srgb_f) {
    std::vector<double>   factors;
    std::vector<inc_lRGB> lRGBs;
    std::vector<double>   luminances;
    factors.reserve(colors_srgb_f.size());
    lRGBs.reserve(colors_srgb_f.size());
    luminances.reserve(colors_srgb_f.size());

    for (size_t i = 0; i < colors_srgb_f.size(); ++i) {
        lRGBs.push_back(Converter::sRGB_to_lRGB(colors_srgb_f[i].first));
        luminances.push_back(Converter::luminance_from_lRGB(lRGBs.back()));
        factors.push_back(colors_srgb_f[i].second);
    }

    return detail::pigment_blend_HLPR(factors, lRGBs, luminances);
}

inline constexpr Color pigment::blend(const std::vector<inc_sRGB> &colors_srgb) {
    std::vector<double>   factors(colors_srgb.size(), 1.0);
    std::vector<inc_lRGB> lRGBs;
    std::vector<double>   luminances;
    factors.reserve(colors_srgb.size());
    lRGBs.reserve(colors_srgb.size());
    luminances.reserve(colors_srgb.size());

    for (size_t i = 0; i < colors_srgb.size(); ++i) {
        lRGBs.push_back(Converter::sRGB_to_lRGB(colors_srgb[i]));
        luminances.push_back(Converter::luminance_from_lRGB(lRGBs.back()));
    }

    return detail::pigment_blend_HLPR(factors, lRGBs, luminances);
}

template <typename... PR_SRGBs_F>
requires(sizeof...(PR_SRGBs_F) > 1) &&
        (std::same_as<std::remove_cvref_t<PR_SRGBs_F>, std::pair<inc_sRGB, double>> && ...)
constexpr Color pigment::blend(const PR_SRGBs_F &...colors) {
    std::vector<double>   factors;
    std::vector<inc_lRGB> lRGBs;
    std::vector<double>   luminances;
    factors.reserve(sizeof...(PR_SRGBs_F));
    lRGBs.reserve(sizeof...(PR_SRGBs_F));
    luminances.reserve(sizeof...(PR_SRGBs_F));

    // Pack expansion to push all we need from the pack inside the vectors
    ((lRGBs.push_back(Converter::sRGB_to_lRGB(colors.first)),
      luminances.push_back(Converter::luminance_from_lRGB(lRGBs.back())), factors.push_back(colors.second)),
     ...);

    return detail::pigment_blend_HLPR(factors, lRGBs, luminances);
}

template <typename... SRGBs>
requires(sizeof...(SRGBs) > 1) && (std::same_as<std::remove_cvref_t<SRGBs>, inc_sRGB> && ...)
constexpr Color pigment::blend(const SRGBs &...colors) {
    std::vector<double>   factors(sizeof...(SRGBs), 1.0);
    std::vector<inc_lRGB> lRGBs;
    std::vector<double>   luminances;
    lRGBs.reserve(sizeof...(SRGBs));
    luminances.reserve(sizeof...(SRGBs));

    // Pack expansion to push all we need from the pack inside the vectors
    ((lRGBs.push_back(Converter::sRGB_to_lRGB(colors.first)),
      luminances.push_back(Converter::luminance_from_lRGB(lRGBs.back()))),
     ...);

    return detail::pigment_blend_HLPR(factors, lRGBs, luminances);
}


// Generates a palette of colors transitioning between two colors.
inline constexpr std::vector<Color> pigment::palette(Color const &a, Color const &b, size_t const sz) {
    std::vector<Color> out;
    out.reserve(sz);
    for (int i = 0; i < sz; ++i) { out.push_back(pigment::blend({{a, double(sz - 1 - i)}, {b, double(i)}})); }
    return out;
}

// Interpolates between multiple colors based on a parameter t.
// Each additional argument should be an array with two elements: [Color, position].
inline constexpr Color pigment::gradient(double t, const std::vector<std::pair<Color, double>> &stops) {
    std::pair<Color, double> a      = {Color(inc_sRGB{0, 0, 0}), 0.0};
    std::pair<Color, double> b      = a;
    bool                     foundA = false, foundB = false;
    for (auto const &pr : stops) {
        if (! foundA || pr.second > a.second) {
            if (pr.second <= t) {
                a      = pr;
                foundA = true;
            }
        }
        if (! foundB || pr.second < b.second) {
            if (pr.second >= t) {
                b      = pr;
                foundB = true;
            }
        }
    }
    if (! foundA) { return b.first; }
    if (! foundB) { return a.first; }
    if (a.second == b.second) { return a.first; }
    double f = (t - a.second) / (b.second - a.second);
    return pigment::blend({{a.first, 1.0 - f}, {b.first, f}});
}


inline constexpr Color detail::pigment_blend_HLPR(std::vector<double> const   &factors,
                                                  std::vector<inc_lRGB> const &lRGBs,
                                                  std::vector<double> const   &luminances) {
    if (factors.size() == 0 || factors.size() != lRGBs.size() || lRGBs.size() != luminances.size()) {
        std::exit(1); // Impossible, we are calling the enclosing lambda wrong
    }

    std::vector<arr_dbl38> KSs;
    KSs.reserve(lRGBs.size());
    for (auto const &lrgb : lRGBs) { KSs.push_back(Converter::KS_from_lRGB(lrgb)); }

    arr_dbl38 Rm{};
    for (int i = 0; i < detail::SAMPLE_SIZE; ++i) {
        double ksSum   = 0.0;
        double concSum = 0.0;
        for (size_t j = 0; j < KSs.size(); ++j) {
            double const conc  = std::pow(factors[j], 2) * luminances[j];
            concSum           += conc;
            ksSum             += KSs[j][i] * conc;
        }
        Rm[i] = detail::KM_func(ksSum / concSum);
    }
    return Color(Rm);
};

} // namespace incom::standard::color

#ifndef INCOM_INCCOL_NAMESPACE_ALIAS
#define INCOM_INCCOL_NAMESPACE_ALIAS
namespace inccol = incom::standard::color;
#endif