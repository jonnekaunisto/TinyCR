// Definition of the Socket class

#ifndef Socket_class
#define Socket_class

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <string>
#include <arpa/inet.h>

const int MAXHOSTNAME = 200;
const int MAXCONNECTIONS = 5;
const int MAXRECV = 500;

class Socket
{
public:
  Socket();
  virtual ~Socket();

  // Server initialization
  int create();
  int bind(const int port);
  int listen() const;
  int accept(Socket &) const;

  // Client initialization
  int connect(const std::string host, const int port);
  int connect(sockaddr_in host, const int port);


  // Data Transimission
  int send(const std::string) const;
  int recv(std::string &) const;

  int send(const char *, const int size) const;
  int recv(char *) const;

  void set_non_blocking(const bool);

  bool is_valid() const { return m_sock != -1; }

  sockaddr_in get_client();

  const Socket& operator << ( const std::string& ) const;
  const Socket& operator >> ( std::string& ) const;

private:
  int m_sock;
  sockaddr_in m_addr;
};

#endif