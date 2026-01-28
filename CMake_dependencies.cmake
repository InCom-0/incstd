include(cmake/CPM.cmake)

if(NOT CPM_USE_LOCAL_PACKAGES)
    set(CPM_USE_LOCAL_PACKAGES ON)
endif()

CPMAddPackage("gh:MiSo1289/more_concepts#master")


find_package(xxHash QUIET)
if(NOT xxHash_FOUND)
    # Ensure CMake can find your FindxxHash.cmake
    list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake/modules")
    find_package(xxHash QUIET)
endif()

if(NOT xxHash_FOUND)
    CPMAddPackage(
        URI "gh:Cyan4973/xxHash@0.8.3"
        SOURCE_SUBDIR cmake_unofficial
        OPTIONS "BUILD_SHARED_LIBS OFF" "XXHASH_BUILD_XXHSUM OFF"
        NAME xxHash
    )
endif()

CPMAddPackage(
    URI "gh:martinus/unordered_dense@4.5.0"
    NAME unordered_dense)
