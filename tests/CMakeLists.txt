# option(INCSTD_BUILD_SHARED_LIB "Build a shared version of incstd" ON)

#####################################################################
### Platform specifics + hacks ###
#####################################################################

######################################################################
### Target specification ###
######################################################################

add_executable(incstd_tests)
target_compile_features(incstd_tests PRIVATE cxx_std_23)
target_sources(incstd_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/pathfinders.cpp)

target_sources(incstd_tests
	PUBLIC
	FILE_SET pub_headers
	TYPE
	HEADERS
	BASE_DIRS
	include
)

##########################################################################
### Dependencies and library linking  ###
##########################################################################
CPMAddPackage(
	URL
		"https://github.com/google/googletest/releases/download/v1.18.0/googletest-1.18.0.tar.gz"
		URL_HASH
		SHA256=6e3191c1455468b3fc35a417fb565c1c5071aee1b7e7f85e30cf48a98d37d8b5
	OPTIONS "gtest_force_shared_crt ON"
	NAME googletest
)

target_link_libraries(incstd_tests PRIVATE incstd gtest_main gmock_main)
