#pragma once

#include <utility>

namespace incom::standard::buffers {
using namespace incom::standard;

/* Simple 'double buffer' class that can be useful when employing iterative algorithms that simply 'mutate' the input in
each iteration. Particularly useful for non-trivial data structures ... for instance containers of containers.
*/
template <typename T>
class doubleBuffer {
private:
    T __dataA;
    T __dataB;

    T *current = &__dataA;
    T *next    = &__dataB;

public:
    doubleBuffer() : __dataA(T()), __dataB(T()), current(&__dataA), next(&__dataB) {};
    doubleBuffer(T initial_data) : __dataA(initial_data), __dataB(initial_data), current(&__dataA), next(&__dataB) {};

    T &getCurrent() const { return (*current); };
    T &getNext() const { return (*next); }

    void swapBuffers() { std::swap(current, next); }
};


} // namespace incom::standard::buffers