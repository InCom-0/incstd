include(cmake/CPM_0.42.1.cmake)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/incom/modules")

include(CMakePushCheckState)
include(CheckCXXSourceCompiles)


set(INCSTD_USE_BUNDLED_MDSPAN 0)

cmake_push_check_state(RESET)
set(CMAKE_REQUIRED_QUIET ON)

if(DEFINED CMAKE_CXX_EXTENSIONS AND NOT CMAKE_CXX_EXTENSIONS)
    set(_incstd_mdspan_standard_flag "${CMAKE_CXX23_STANDARD_COMPILE_OPTION}")
else()
    set(_incstd_mdspan_standard_flag "${CMAKE_CXX23_EXTENSION_COMPILE_OPTION}")
endif()

if(_incstd_mdspan_standard_flag)
    string(APPEND CMAKE_REQUIRED_FLAGS " ${_incstd_mdspan_standard_flag}")
endif()

check_cxx_source_compiles(
    [[
        #include <cstddef>
        #include <mdspan>

        int main() {
                std::mdspan<int, std::dextents<std::size_t, 1>> view(nullptr, 0);
                return static_cast<int>(view.rank());
        }
    ]]
    INCSTD_HAS_STD_MDSPAN)

cmake_pop_check_state()

if(INCSTD_HAS_STD_MDSPAN)
    message(STATUS "incstd: detected standard <mdspan> support")
else()
    message(STATUS "incstd: standard <mdspan> is unavailable; using bundled mdspan fallback")
    CPMAddPackage("gh:InCom-0/mdspan#stable")
    set(INCSTD_USE_BUNDLED_MDSPAN 1)
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
