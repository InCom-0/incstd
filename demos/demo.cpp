#include <incstd.hpp>
#include <print>


int main(int argc, char *argv[]) {

    using namespace incom::standard::coroutines;

    std::vector<size_t> const testVec{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13};
    // std::vector<size_t>       testVec2(14, 0uz);
    // auto                      rv = incstd::containers::RingVector(testVec2);

    // for (size_t i = 0; i < 6; ++i) {
    //     rv.update_preRotate(i);

    //     for (auto const &itr : rv) { std::print("{}, ", itr); }
    //     std::print("\n");
    // }
    // static_assert(std::forward_iterator<incom::standard::containers::detail::_RingVector_iter<size_t>>);


    

}
