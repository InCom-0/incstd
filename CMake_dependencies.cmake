include(cmake/CPM.cmake)

CPMAddPackage("gh:InCom-0/more_concepts#master")
CPMAddPackage(
    URI "gh:Cyan4973/xxHash@0.8.3"
    SOURCE_SUBDIR cmake_unofficial
)
CPMAddPackage("gh:martinus/unordered_dense@4.5.0")