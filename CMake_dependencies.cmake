include(cmake/lefticus/CPM.cmake)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/incom/modules")


CPMAddPackage("gh:MiSo1289/more_concepts#master")

# Try again with CPM, if not found either then build from source
CPMAddPackage(
    URI "gh:Cyan4973/xxHash#dev"
    SOURCE_SUBDIR build/cmake
    OPTIONS "BUILD_SHARED_LIBS OFF" "XXHASH_BUILD_XXHSUM OFF"
    NAME xxHash
)

CPMAddPackage(
    URI "gh:martinus/unordered_dense@4.5.0"
    NAME unordered_dense)
