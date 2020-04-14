add_library(utilsLibrary common.cpp farmhash.cpp input_types.cpp helpers.cpp perf_tool.cpp)

SET(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-g -Ofast -std=c++11 -pthread")