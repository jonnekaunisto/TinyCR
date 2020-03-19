#ifndef CLIENTSOCK_H
#define CLIENTSOCK_H

#include <iostream>
#include <future>
#include <functional>

#include <cerrno>
#include <cstring>

#include <strings.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/signal.h>


class ClientSock {
public:
  ClientSock();
  
  ClientSock(std::string host, unsigned int port);
  
  
  explicit ClientSock(int sock);
  
  ~ClientSock();
  
  bool hasError();
  
  int connect(std::string host, unsigned int port);
  
  int disconnect();
  
  int write(std::string mesg);
  
  std::string read();
  
  std::string readAll();
  
  std::string host;
  unsigned int port;
  bool connected;

protected:

private:
  int enable_keepalive(int sock);
  
  static const unsigned int buffSize = 1000;
  int sockfd;//establish connection to ID distribution server
  struct sockaddr_in servaddr;
  char recv[buffSize];
  struct hostent *server;
};

#endif // CLIENTSOCK_H
