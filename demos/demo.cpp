#include <incstd.hpp>
#include <print>

int main(int argc, char *argv[]) {


    std::vector<size_t> testVec{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13};

    auto ttt = incstd::views::combinations_k<3>(testVec);

    for (auto const &tpl : ttt) { std::print("{}, {}, {}\n", std::get<0>(tpl), std::get<1>(tpl), std::get<2>(tpl)); }
}