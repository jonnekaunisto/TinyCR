cmake_minimum_required(VERSION 3.15.5)

# set the project name
project(TinyCR VERSION 1.0 LANGUAGES CXX)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})


add_subdirectory(Socket)
add_subdirectory(utils)

add_executable(server server.cpp)
target_link_libraries(server socketLibrary utilsLibrary)

add_executable(client client.cpp)
target_link_libraries(client socketLibrary utilsLibrary)

add_executable(platform_test platform_test.cpp)
target_link_libraries(platform_test socketLibrary utilsLibrary)


# specify the C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)
SET(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-g -Ofast -std=c++11 -pthread")
