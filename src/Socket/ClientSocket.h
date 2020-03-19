// Definition of the ClientSocket class

#ifndef ClientSocket_class
#define ClientSocket_class

#include "Socket.h"


class ClientSocket : public Socket
{
 public:

  ClientSocket ( std::string host, int port );
  ClientSocket ( sockaddr_in host, int port );
  virtual ~ClientSocket(){};

};


#endif