#####################################################################
### Platform specifics + hacks ###
#####################################################################

######################################################################
### Target specification ###
######################################################################

add_executable(incstd_bench)
target_compile_features(incstd_bench PRIVATE cxx_std_23)
target_sources(incstd_bench PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/pathfinders.cpp)

target_sources(incstd_bench
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
	URI
	"gh:martinus/nanobench@4.6.0"
	NAME nanobench
)
target_link_libraries(incstd_bench PRIVATE incstd nanobench)