cmake_minimum_required(VERSION 3.30...4.0)

project(incstd)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

include(CMake_dependencies.cmake) ### Loads and declares the requires external libraries using both vcpkg and CMake's FetchContent.


add_library(incstd INTERFACE)
add_library(incstd::incstd ALIAS incstd)
target_link_libraries(incstd INTERFACE more_concepts::more_concepts xxhash unordered_dense::unordered_dense)

target_sources(incstd
    PUBLIC
        FILE_SET pub_headers
        TYPE HEADERS
        BASE_DIRS
                include)
                
target_compile_features(incstd INTERFACE cxx_std_23)

option(INCSTD_BUILD_DEMOS "Build demo executables" ${PROJECT_IS_TOP_LEVEL})
if(INCSTD_BUILD_DEMOS)
    add_subdirectory(demos)
endif()