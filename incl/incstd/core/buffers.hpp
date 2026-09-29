#pragma once

#include <algorithm>
#include <utility>


namespace incom::standard::buffers {
using namespace incom::standard;


template <typename T, std::size_t NUM = 2>
requires(NUM > 1) && std::is_default_constructible_v<T>
class MultiBuffer {
private:
    std::array<T, NUM> __data;

    std::array<T *, NUM> __dataPTRs = [&]<typename SZ, SZ... ints>(const std::integer_sequence<SZ, ints...> &) {
        return std::array<T *, NUM>{&__data[ints]...};
    }(std::make_index_sequence<NUM>{});

public:
    // CONSTRUCTION
    MultiBuffer()
        : __data([]<typename SZ, SZ... ints>(const std::integer_sequence<SZ, ints...> &) {
              return std::array<T, NUM>{(ints, T{})...};
          }(std::make_index_sequence<NUM>{})) {};

    // By default 'initial_data' gets only copied into 'Current', rest is default constructed
    template <bool fillAll = false>
    MultiBuffer(T const &initial_data)
        : __data([&]<typename SZ, SZ... ints>(const std::integer_sequence<SZ, ints...> &) {
              if constexpr (fillAll) { return std::array<T, NUM>{(ints, initial_data)...}; }
              else {
                  auto res    = std::array<T, NUM>{(ints, T{})...};
                  res.front() = initial_data;
                  return res;
              }
          }(std::make_index_sequence<NUM>{})) {};

    MultiBuffer(MultiBuffer const &other) = delete;
    MultiBuffer(MultiBuffer &&other)      = delete;


    // GETTING THE CONTAINED DATA
    T &
    getCurrent() const {
        return *__dataPTRs[0];
    }
    T &
    getNext() const {
        return *__dataPTRs[1];
    }

    template <std::size_t ID>
    requires(ID < NUM)
    T &
    getNth() const {
        return *__dataPTRs[ID];
    }

    // SWAPPING / ROTATING
    // 'Next' becomes 'Current', 'Current' goes to last, others similarly
    void
    rotate() {
        std::ranges::rotate(__dataPTRs, __dataPTRs.begin() + 1);
    }

    void
    rotate_reverse() {
        std::ranges::rotate(__dataPTRs, __dataPTRs.end() - 1);
    }

    // Convenience 'alias' which just calls rotate()
    // More syntactically friedly (especially when using the class as 'double buffer')
    void
    swap_buffers() {
        rotate();
    }
};

template <typename T>
using DoubleBuffer = MultiBuffer<T, 2uz>;

template <typename T>
using TrippleBuffer = MultiBuffer<T, 3uz>;


} // namespace incom::standard::buffers