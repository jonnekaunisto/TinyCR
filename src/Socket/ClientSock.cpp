#include "ClientSock.h"
using namespace std;

ClientSock::ClientSock(string host, unsigned int port) {
  connect(host, port);
}

ClientSock::ClientSock() {
  connected = false;
}

ClientSock::ClientSock(int sock) {
  sockfd = sock;
  connected = true;
}

ClientSock::~ClientSock() {
  disconnect();
}

int ClientSock::connect(string host, unsigned int port) {
  ClientSock::host = host;
  ClientSock::port = port;
  
  bzero(&servaddr, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  server = gethostbyname(host.data());
  bcopy((char *) server->h_addr, (char *) &servaddr.sin_addr.s_addr, server->h_length);
  servaddr.sin_port = htons(port);
  
  if (connected)
    disconnect();
  
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  
  /*struct timeval tv;
  tv.tv_sec = 5;//5 Secs Timeout
  tv.tv_usec = 0;//Not init'ing this can cause strange errors
  setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char*)&tv, sizeof(struct timeval));*/
  
  enable_keepalive(sockfd);
  
  for (size_t i = 0; i < 3; i++) { //try to connect 3 times
    if (::connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0)
      cerr << "Error on connecting: " << errno << "  " << strerror(errno) << endl;
    else {
      connected = true;
      return 0;
    }
  }
  
  connected = false;
  return 1;
}

bool ClientSock::hasError() {
  if (sockfd == -1)
    return true;
  
  int error = 0;
  socklen_t len = sizeof(error);
  int retval = getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &len);
  
  return retval != 0 || error != 0;
}

int ClientSock::enable_keepalive(int sock) {
  int yes = 1;
  
  if (setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &yes, sizeof(int)) == -1)
    return -1;
  
  int idle = 1;
  
  if (setsockopt(sock, IPPROTO_TCP, TCP_KEEPIDLE, &idle, sizeof(int)) == -1)
    return -1;
  
  int interval = 1;
  
  if (setsockopt(sock, IPPROTO_TCP, TCP_KEEPINTVL, &interval, sizeof(int)) == -1)
    return -1;
  
  int maxpkt = 10;
  
  if (setsockopt(sock, IPPROTO_TCP, TCP_KEEPCNT, &maxpkt, sizeof(int)) == -1)
    return -1;
  
  return 0;
}

int ClientSock::disconnect() {
  if (!connected)
    return -1;
  
  close(sockfd);
  connected = false;
  
  return 0;
}

int ClientSock::write(string mesg) {
  if (!connected)
    return 1;
  
  timeval tv{};
  tv.tv_sec = 10;
  tv.tv_usec = 0;
  fd_set writefds;
  FD_ZERO(&writefds);
  FD_SET(sockfd, &writefds);
  
  //cout << "w: " << mesg << endl;
  
  ssize_t sentBytes = 0;
  
  for (size_t i = 0; i < mesg.length(); i += sentBytes) {
    FD_ZERO(&writefds);
    FD_SET(sockfd, &writefds);
    int rv = select(sockfd + 1, NULL, &writefds, NULL, &tv);
    
    if (rv == -1)
      cerr << errno << "  " << strerror(errno) << endl;
    else if (rv == 0)
      sentBytes = 0;
    else if (rv > 0 && FD_ISSET(sockfd, &writefds)) {
      sentBytes = ::write(sockfd, mesg.substr(i, mesg.length() - i).c_str(), mesg.length() - i);
      
      if (sentBytes == -1) {
        cerr << "Error sending IDs: " << errno << "  " << strerror(errno) << endl;
        return 1;
      }
    }
  }
  
  return 0;
}

string ClientSock::read() {
  if (!connected)
    return "";
  
  timeval tv;
  tv.tv_sec = 10;
  tv.tv_usec = 0;
  fd_set readfds;
  FD_ZERO(&readfds);
  FD_SET(sockfd, &readfds);
  
  string resp = "";
  unsigned int n = 0;
  
  do {
    FD_ZERO(&readfds);
    FD_SET(sockfd, &readfds);
    int rv = select(sockfd + 1, &readfds, nullptr, NULL, &tv);
    
    if (rv <= -1)
      cerr << errno << "  " << strerror(errno) << endl;
    else if (rv == 0)
      break;
    else if (rv > 0 && FD_ISSET(sockfd, &readfds)) {
      ssize_t tn = ::read(sockfd, recv, buffSize - 1);      //avoid signcompare warning
      
      if (tn > 0) {
        n = tn;
        recv[n] = '\0';
        string tResp(recv, n);
        resp += tResp;
      } else if (tn == -1) {
        if (errno == 11) {
          //get the good part of the received stuff also if the connection closed during receive -> happens sometimes with short messages
          string tResp(recv);
          
          if (tResp.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ01234567890") ==
              std::string::npos) //but only allow valid chars
            resp += tResp;
        } else {
          cerr << errno << "  " << strerror(errno) << endl;
        }
        
        break;
      } else {
        break;
      }
    } else
      cerr << "ERROR: rv: " << rv << endl;
    
  } while (n >= buffSize - 1);
  
  //if(resp != "")
  //cout << "r: " << resp << endl;
  
  return resp;
}

string ClientSock::readAll() {
  string full = read();
  
  while (full.find("END") == string::npos)
    full += read();
  
  full = full.substr(0, full.find("END"));
  
  return full;
}
