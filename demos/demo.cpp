#include <iostream>

#include <incstd/core/concepts.hpp>
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

    auto ttt  = incom::standard::filesys::locations::cache_dir(true);
    auto ttt2 = incom::standard::filesys::locations::cache_dir("incplot",true);

    if (ttt) { std::cout << ttt->generic_string() << '\n'; }

    if (ttt2) { std::cout << ttt2->generic_string() << '\n'; }


    auto curPalette = ColorQuery::get_palette256();

    // std::cout << "Palette16:\n";
    // for (auto &item : ttt.value()) {
    //     std::cout << static_cast<int>(item[0]) << ", " << static_cast<int>(item[1]) << ", " <<
    //     static_cast<int>(item[2])
    //               << '\n';
    // }

    if (not curPalette.has_value()) {
        std::cout << "There was an error querying the terminal\n";
        return 1;
    }


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

    using namespace incom::standard::concepts;

    // std::expected<size_t, std::error_code> expTest_1;

    // std::cout << is_specialization_of<decltype(expTest_1), std::expected> << '\n';
    // std::cout << is_specialization_of<std::expected<size_t, std::error_code>, std::expected> << '\n';
    // std::cout << is_specialization_of<std::expected<size_t, std::error_code>, std::optional> << '\n';
    // std::cout
    //     << is_specialization_of<std::expected<volatile const size_t &, std::error_code>, std::expected> << '\n';


    return 1;
}
