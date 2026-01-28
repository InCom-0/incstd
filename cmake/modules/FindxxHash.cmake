find_package(PkgConfig QUIET)

if(PkgConfig_FOUND)
    pkg_check_modules(XXHASH IMPORTED_TARGET libxxhash)
endif()

# Fallback if the IMPORTED target wasn't created
if(XXHASH_FOUND AND NOT TARGET PkgConfig::XXHASH)
    add_library(PkgConfig::XXHASH INTERFACE IMPORTED)
    target_include_directories(PkgConfig::XXHASH INTERFACE ${XXHASH_INCLUDE_DIRS})
    target_link_libraries(PkgConfig::XXHASH INTERFACE ${XXHASH_LIBRARIES})
endif()

if(TARGET PkgConfig::XXHASH)
    add_library(xxHash::xxhash ALIAS PkgConfig::XXHASH)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(xxHash DEFAULT_MSG
    XXHASH_FOUND
    XXHASH_LIBRARIES
    XXHASH_INCLUDE_DIRS
)

set(xxHash_FOUND ${xxHash_FOUND})