#include <incstd.hpp>
#include <iterator>
#include <print>

int main(int argc, char *argv[]) {


    std::vector<size_t> const testVec{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13};
    std::vector<size_t>       testVec2(14, 0uz);
    auto                      rv = incstd::containers::RingVector(testVec2);

    for (size_t i = 0; i < 6; ++i) {
        rv.update_preRotate(i);

        for (auto const &itr : rv) { std::print("{}, ", itr); }
        std::print("\n");
    }
    static_assert(std::forward_iterator<incom::standard::containers::detail::_RingVector_iter<size_t>>);


    for (auto const &tpl : incstd::views::combinations_k<4>(testVec) | std::views::drop(0)) {
        std::print("{}, {}, {}, {}\n", std::get<0>(tpl), std::get<1>(tpl), std::get<2>(tpl), std::get<3>(tpl));
    }
}