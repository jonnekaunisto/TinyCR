// Implementation of the ServerSocket class

#include "ServerSocket.h"
#include "SocketException.h"
#include <string.h>


ServerSocket::ServerSocket ( int port )
{
    int errornum = Socket::create();
    if (errornum != 0)
    {
        throw SocketException(strerror(errornum));
    }

    errornum = Socket::bind ( port );
    if (errornum != 0)
    {
        throw SocketException(strerror(errornum));
    }

    errornum = Socket::listen();
    if (errornum != 0)
    {
        throw SocketException(strerror(errornum));
    }
}

ServerSocket::~ServerSocket()
{
}

void ServerSocket::accept ( ServerSocket& sock )
{
    int errornum = Socket::accept(sock);
    if (errornum != 0)
    {
        throw SocketException(strerror(errornum));
    }
}