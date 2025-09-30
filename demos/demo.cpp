#include <iostream>

#include <incstd/incstd_all.hpp>

int main(int argc, char *argv[]) {


    using namespace incom::standard::console;
    using namespace std::literals;

    // auto fg = ColorQuery::get_foreground();
    // if (fg) {
    //     auto [r, g, b] = fg.value();
    //     std::cerr << "Foreground RGB: " << static_cast<int>(r) << "," << static_cast<int>(g) << ","
    //               << static_cast<int>(b) << "\n";
    // }
    // else {
    //     std::cerr << "Could not query FG: " << to_string(fg.error()) << "\n";
    //     // fallback logic here (e.g. assume defaults or use other config)
    // }

    // // Query palette index 1 (ANSI red)

    // for (size_t id = 0; id < 16; ++id) {
    //     auto idx = ColorQuery::get_paletteIdx(id);
    //     if (idx) {
    //         auto [r, g, b] = idx.value();

    //         std::cout << "Index " << id << ": " << static_cast<int>(r) << "," << static_cast<int>(g) << ","
    //                   << static_cast<int>(b) << "\n";
    //     }
    //     else { std::cerr << "Palette query failed: " << to_string(idx.error()) << "\n"; }
    // }


    auto curPalette = ColorQuery::get_palette256();

    // std::cout << "Palette16:\n";
    // for (auto &item : ttt.value()) {
    //     std::cout << static_cast<int>(item[0]) << ", " << static_cast<int>(item[1]) << ", " <<
    //     static_cast<int>(item[2])
    //               << '\n';
    // }
    std::cout << "Palette256:\n";
    for (auto &item : curPalette.value()) {
        std::cout << static_cast<int>(item.r) << ", " << static_cast<int>(item.g) << ", " << static_cast<int>(item.b)
                  << '\n';
    }

    std::cout << ANSI::SGR_builder()
                     .bold()
                     .italic()
                     .faint()
                     .underline()
                     .overline()
                     .color_bg(curPalette->at(80))
                     .color_fg(curPalette->at(5))
                     .add_string("Hello World"sv)
                     .reset_all();

    return 1;
}
