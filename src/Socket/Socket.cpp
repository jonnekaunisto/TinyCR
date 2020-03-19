// Implementation of the Socket class.


#include "Socket.h"
#include "SocketException.h"
#include "string.h"
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>



Socket::Socket() :
  m_sock(-1)
{

  memset(&m_addr,
     0,
     sizeof(m_addr));

}

Socket::~Socket()
{
  if(is_valid())
    ::close(m_sock);
}

int Socket::create()
{
    m_sock = socket(AF_INET, SOCK_STREAM, 0);

    if(!is_valid())
    {
        return -1;
    }

    // TIME_WAIT - argh
    int on = 1;
    if(setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR,(const char*) &on, sizeof(on)) == -1)
    {
        return errno;
    }

    return 0;
}



int Socket::bind(const int port)
{
    if(! is_valid())
    {
        return -1;
    }

    m_addr.sin_family = AF_INET;
    m_addr.sin_addr.s_addr = INADDR_ANY;
    m_addr.sin_port = htons(port);

    int bind_return = ::bind(m_sock,(struct sockaddr*) &m_addr, sizeof(m_addr));


    if(bind_return == -1)
    {
        return errno;
    }

    return 0;
}


int Socket::listen() const
{
    if(!is_valid())
    {
        return -1;
    }

    int listen_return = ::listen(m_sock, MAXCONNECTIONS);

    if(listen_return == -1)
    {
        return errno;
    }

    return 0;
}

int Socket::accept(Socket& new_socket) const
{
    int addr_length = sizeof(m_addr);
    new_socket.m_sock = ::accept(m_sock,(sockaddr *) &m_addr,(socklen_t *) &addr_length);
    new_socket.m_addr = m_addr;
    if(new_socket.m_sock <= 0)
        return errno;
    else
        return 0;
}


int Socket::send(const std::string s) const
{
  return ::send(m_sock, s.c_str(), s.size(), 0);
}

int Socket::send(const char* s, const int size) const
{
  return ::send(m_sock, s, size, 0); 
}


int Socket::recv(std::string& s) const
{
  char buf [ MAXRECV + 1 ];

  s = "";

  memset(buf, 0, MAXRECV + 1);

  int status = ::recv(m_sock, buf, MAXRECV, 0);

  if(status == -1)
    {
      //cout << "status == -1   errno == " << errno << "  in Socket::recv\n";
      return -1;
    }
  else if(status == 0)
    {
      return 0;
    }
  else
    {
      s = buf;
      return status;
    }
}

int Socket::recv(char* s) const
{
  //s = new char[MAXRECV + 1];
  memset(s, 0, MAXRECV + 1);

  int status = ::recv ( m_sock, s, MAXRECV, 0 );
  if ( status == -1 )
    {
      //cout << "status == -1   errno == " << errno << "  in Socket::recv\n";
      return 0;
    }
  else if ( status == 0 )
    {
      return 0;
    }
  else
    {
      return status;
    }
}



int Socket::connect(const std::string host, const int port)
{
  if(! is_valid()) return false;

  m_addr.sin_family = AF_INET;
  m_addr.sin_port = htons(port);

  int status = inet_pton(AF_INET, host.c_str(), &m_addr.sin_addr);

  if(errno == EAFNOSUPPORT) return false;

  status = ::connect(m_sock,(sockaddr *) &m_addr, sizeof(m_addr));

  if(status == 0)
  {
    return 0;
  }
  else
  {
    return errno;
  }
}

int Socket::connect(sockaddr_in host, const int port)
{
  if(! is_valid()) return false;

  host.sin_family = AF_INET;
  host.sin_port = htons(port);

  int status = ::connect(m_sock,(sockaddr *) &m_addr, sizeof(m_addr));

  if(status == 0)
  {
    return 0;
  }
  else
  {
    return errno;
  }
}

void Socket::set_non_blocking(const bool b)
{

  int opts;

  opts = fcntl(m_sock,
     F_GETFL);

  if(opts < 0)
    {
      return;
    }

  if(b)
    opts =(opts | O_NONBLOCK);
  else
    opts =(opts & ~O_NONBLOCK);

  fcntl(m_sock,
    F_SETFL,opts);

}

sockaddr_in Socket::get_client()
{
  return m_addr;
}


const Socket& Socket::operator <<(const std::string& s) const
{
    int bytesSent = Socket::send(s);
    if(bytesSent == -1)
    {
        throw SocketException(strerror(errno));
    }

    return *this;

}


const Socket& Socket::operator >>(std::string& s) const
{
  int bytesReceived = Socket::recv(s);
  if(bytesReceived == -1)
  {
      throw SocketException(strerror(errno));
  }

  return *this;
}