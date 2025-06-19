add_executable(incstd_demo ${CMAKE_CURRENT_SOURCE_DIR}/demo.cpp)
target_link_libraries(incstd_demo PRIVATE incstd "-lstdc++exp")
target_compile_features(incstd_demo PRIVATE cxx_std_23)