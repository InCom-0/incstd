#include <iostream>

#include <incstd_all.hpp>

int main(int argc, char *argv[]) {


    using namespace incom::standard::console;

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


    auto ttt  = ColorQuery::get_palette16();
    auto ttt2 = ColorQuery::get_palette256_TMP();

    // std::cout << "Palette16:\n";
    // for (auto &item : ttt.value()) {
    //     std::cout << static_cast<int>(item[0]) << ", " << static_cast<int>(item[1]) << ", " << static_cast<int>(item[2])
    //               << '\n';
    // }
    std::cout << "Palette256:\n";
    for (auto &item : ttt2.value()) {
        std::cout << static_cast<int>(item[0]) << ", " << static_cast<int>(item[1]) << ", " << static_cast<int>(item[2])
                  << '\n';
    }

    return 1;
}
