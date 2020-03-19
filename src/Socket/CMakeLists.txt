add_library(socketLibrary Socket.cpp ClientSock.cpp ClientSocket.cpp ServerSocket.cpp)

SET(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-pg -g -Ofast -std=c++11 -pthread")
