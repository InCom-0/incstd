include(FetchContent)

FetchContent_Declare(
    more_concepts
    GIT_REPOSITORY https://github.com/MiSo1289/more_concepts.git
    GIT_TAG origin/master
)
FetchContent_MakeAvailable(more_concepts)


FetchContent_Declare(
    xxhash
    GIT_REPOSITORY https://github.com/Cyan4973/xxHash.git
    GIT_TAG origin/dev
)
FetchContent_MakeAvailable(xxhash)


FetchContent_Declare(
    unordered_dense
    GIT_REPOSITORY https://github.com/martinus/unordered_dense.git
    GIT_TAG v4.5.0
)
FetchContent_MakeAvailable(unordered_dense)