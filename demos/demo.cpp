#include <iostream>

#include <incstd_all.hpp>

int main(int argc, char *argv[]) {


    using namespace incom::standard::console;

    auto fg = QueryColor::queryForeground();
    if (fg) {
        auto [r, g, b] = fg.value();
        std::cerr << "Foreground RGB: " << static_cast<int>(r) << "," << static_cast<int>(g) << ","
                  << static_cast<int>(b) << "\n";
    }
    else {
        std::cerr << "Could not query FG: " << to_string(fg.error()) << "\n";
        // fallback logic here (e.g. assume defaults or use other config)
    }

    // Query palette index 1 (ANSI red)

    for (size_t id = 0; id < 16; ++id) {
        auto idx = QueryColor::queryPaletteIndex(id);
        if (idx) {
            auto [r, g, b] = idx.value();

            std::cout << "Index " << id << ": " << static_cast<int>(r) << "," << static_cast<int>(g) << ","
                      << static_cast<int>(b) << "\n";
        }
        else { std::cerr << "Palette query failed: " << to_string(idx.error()) << "\n"; }
    }
}
