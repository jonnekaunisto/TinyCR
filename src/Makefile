# Makefile for the socket programming example
#

simple_server_objects = ClientSocket.o ServerSocket.o Socket.o server.o
simple_client_objects = ClientSocket.o ServerSocket.o Socket.o client.o

QMAKE_CXXFLAGS += -std=c++14
all : simple_server simple_client

simple_server: $(simple_server_objects)
	g++ -o simple_server $(simple_server_objects)


simple_client: $(simple_client_objects)
	g++ -o simple_client $(simple_client_objects)


Socket: Socket.cpp
ServerSocket: ServerSocket.cpp
ClientSocket: ClientSocket.cpp
simple_server_main: simple_server_main.cpp
simple_client_main: simple_client_main.cpp


clean:
	rm -f *.o simple_server simple_client