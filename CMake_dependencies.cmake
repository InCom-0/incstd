include(cmake/CPM.cmake)

if(BUILD_SHARED_LIBS)
    set(CPM_USE_LOCAL_PACKAGES ON)
endif()

CPMAddPackage("gh:MiSo1289/more_concepts#master")
CPMAddPackage(
    URI "gh:Cyan4973/xxHash@0.8.3"
    SOURCE_SUBDIR cmake_unofficial
    NAME xxHash
)
CPMAddPackage(
    URI "gh:martinus/unordered_dense@4.5.0"
    NAME unordered_dense)
