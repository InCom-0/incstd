include(cmake/CPM_0.43.1.cmake)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/incom/modules")


######################################
### MDSPAN sourcing if necessary
######################################

# Provider policy:
# AUTO    -> use stdlib only if BOTH mdspan + submdspan are available at compile-time
# STDLIB  -> force stdlib (error at compile-time if missing required features)
# BUNDLED -> force bundled
set(INCSTD_MDSPAN_PROVIDER "AUTO" CACHE STRING "AUTO|STDLIB|BUNDLED")
set_property(CACHE INCSTD_MDSPAN_PROVIDER PROPERTY STRINGS AUTO STDLIB BUNDLED)

# If not forced STDLIB, make bundled fallback available.
if(NOT INCSTD_MDSPAN_PROVIDER STREQUAL "STDLIB")
	CPMAddPackage("gh:InCom-0/mdspan#stable")
	set(INCSTD_MDSPAN_TARGET mdspan::mdspan)
endif()

# Export one compile define for headers to consume.
set(INCSTD_MDSPAN_PROVIDER_DEFINE "")
if(INCSTD_MDSPAN_PROVIDER STREQUAL "STDLIB")
	set(INCSTD_MDSPAN_PROVIDER_DEFINE INCSTD_MDSPAN_FORCE_STDLIB=1)
elseif(INCSTD_MDSPAN_PROVIDER STREQUAL "BUNDLED")
	set(INCSTD_MDSPAN_PROVIDER_DEFINE INCSTD_MDSPAN_FORCE_BUNDLED=1)
endif()



CPMAddPackage("gh:MiSo1289/more_concepts#master")

# Try again with CPM, if not found either then build from source
CPMAddPackage(
	URI
	"gh:Cyan4973/xxHash#dev"
	SOURCE_SUBDIR build/cmake
	OPTIONS
		"BUILD_SHARED_LIBS OFF"
		"XXHASH_BUILD_XXHSUM OFF"
	NAME xxHash
)

CPMAddPackage(
	URI
	"gh:martinus/unordered_dense@4.8.1"
	NAME unordered_dense
)
