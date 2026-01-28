include(cmake/CPM.cmake)

if(NOT CPM_USE_LOCAL_PACKAGES)
    set(CPM_USE_LOCAL_PACKAGES ON)
endif()

CPMAddPackage("gh:MiSo1289/more_concepts#master")

# Try regular find
find_package(xxHash QUIET)

# If not inject local script that internally uses PkgConfig
if(NOT xxHash_FOUND)
    list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake/modules")
endif()

# Try again with CPM, if not found either then build from source
CPMAddPackage(
    URI "gh:Cyan4973/xxHash@0.8.3"
    SOURCE_SUBDIR cmake_unofficial
    OPTIONS "BUILD_SHARED_LIBS OFF" "XXHASH_BUILD_XXHSUM OFF"
    NAME xxHash
)

CPMAddPackage(
    URI "gh:martinus/unordered_dense@4.5.0"
    NAME unordered_dense)
