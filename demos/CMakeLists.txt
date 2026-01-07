add_executable(incstd_demo ${CMAKE_CURRENT_SOURCE_DIR}/demo.cpp)
target_link_libraries(incstd_demo PRIVATE incstd "-lstdc++exp")
target_compile_features(incstd_demo PRIVATE cxx_std_23)

if(MINGW AND (CMAKE_BUILD_TYPE STREQUAL "Release"))
    target_link_options(incstd_demo PRIVATE -static)
endif()