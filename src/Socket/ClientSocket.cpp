// Implementation of the ClientSocket class

#include "ClientSocket.h"
#include "SocketException.h"
#include <string.h>



ClientSocket::ClientSocket ( std::string host, int port )
{
    int errornum = Socket::create();
    if (errornum != 0)
    {
        throw SocketException(strerror(errornum));
    }

    errornum = Socket::connect (host, port);
    if (errornum != 0)
    {
        throw SocketException (strerror(errornum));
    }

}

ClientSocket::ClientSocket(sockaddr_in host, int port)
{
    int errornum = Socket::create();
    if (errornum != 0)
    {
        throw SocketException(strerror(errornum));
    }

    errornum = Socket::connect(host, port);
    if (errornum != 0)
    {
        throw SocketException(strerror(errornum));
    }
}