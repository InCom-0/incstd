if(NOT DEFINED CPM_USE_LOCAL_PACKAGES)
    set(CPM_USE_LOCAL_PACKAGES ${incstd_USE_LOCAL_PACKAGES} CACHE BOOL "CPM will try to find packages locally first" FORCE)
endif()
if(NOT DEFINED CPM_LOCAL_PACKAGES_ONLY)
    set(CPM_LOCAL_PACKAGES_ONLY ${incstd_USE_LOCAL_PACKAGES_ONLY} CACHE BOOL
        "CPM will not be forbidden from downloading packages. Will have to use local packages." FORCE)
endif()

include(cmake/CPM.cmake)


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
