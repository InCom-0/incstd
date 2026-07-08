include(cmake/CPM_0.43.1.cmake)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/incom/modules")

include(CMakePushCheckState)
include(CheckCXXSourceCompiles)


set(INCSTD_USE_BUNDLED_MDSPAN 0)

cmake_push_check_state(RESET)
# set(CMAKE_REQUIRED_QUIET ON)

if((DEFINED CMAKE_CXX26_STANDARD_COMPILE_OPTION) OR (DEFINED CMAKE_CXX26_EXTENSION_COMPILE_OPTION))
    if(NOT CMAKE_CXX_EXTENSIONS)
        string(APPEND CMAKE_REQUIRED_FLAGS " ${CMAKE_CXX26_STANDARD_COMPILE_OPTION}")
    else()
        string(APPEND CMAKE_REQUIRED_FLAGS " ${CMAKE_CXX26_EXTENSION_COMPILE_OPTION}")
    endif()
else()
    if(NOT CMAKE_CXX_EXTENSIONS)
        string(APPEND CMAKE_REQUIRED_FLAGS " ${CMAKE_CXX23_STANDARD_COMPILE_OPTION}")
    else()
        string(APPEND CMAKE_REQUIRED_FLAGS " ${CMAKE_CXX23_EXTENSION_COMPILE_OPTION}")
    endif()
endif()

check_cxx_source_compiles(
    [[
        #include <vector>
        #include <mdspan>
        int main() {
                std::vector<int> matrix;
                std::mdspan<int, std::dextents<std::size_t, 2>> view(matrix.data(), 3, 3);
                return static_cast<int>(view.rank());
        }
    ]]
    INCSTD_STDLIB_HAS_MDSPAN)

check_cxx_source_compiles(
    [[
        #include <mdspan>
        #include <vector>

        int main() {
                std::vector<int> matrix;
                std::mdspan<int, std::dextents<std::size_t, 2>> view(matrix.data(), 3, 3);
                auto inner = std::submdspan(view, std::pair{1, 1}, std::pair{2, 2});
                
                return static_cast<int>(inner.rank());
        }
    ]]
    INCSTD_STDLIB_HAS_SUBMDSPAN)

cmake_pop_check_state()

set(INCSTD_USE_BUNDLED_MDSPAN 0)
set(INCSTD_USE_BUNDLED_SUBMDSPAN 0)
if(INCSTD_STDLIB_HAS_MDSPAN AND INCSTD_STDLIB_HAS_SUBMDSPAN)
    message(STATUS "incstd: detected standard <mdspan> with 'std::submdspan' support")
else()
    if(INCSTD_STDLIB_HAS_MDSPAN AND NOT INCSTD_STDLIB_HAS_SUBMDSPAN)
        message(STATUS "incstd: standard <mdspan> is available but submdspan is not; using bundled submdspan fallback")
    else()
        message(STATUS "incstd: standard <mdspan> is unavailable; using bundled mdspan fallback")
        set(INCSTD_USE_BUNDLED_MDSPAN 1)
    endif()

    CPMAddPackage("gh:InCom-0/mdspan#stable")
    set(INCSTD_USE_BUNDLED_SUBMDSPAN 1)
    set(INCSTD_MDSPAN_TARGET mdspan::mdspan)
endif()


CPMAddPackage("gh:MiSo1289/more_concepts#master")

# Try again with CPM, if not found either then build from source
CPMAddPackage(
    URI "gh:Cyan4973/xxHash#dev"
    SOURCE_SUBDIR build/cmake
    OPTIONS "BUILD_SHARED_LIBS OFF" "XXHASH_BUILD_XXHSUM OFF"
    NAME xxHash
)

CPMAddPackage(
    URI "gh:martinus/unordered_dense@4.8.1"
    NAME unordered_dense)
