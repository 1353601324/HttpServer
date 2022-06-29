#pragma once
#include <unistd.h>
#include "Sock.hpp"


class tcpServer
{
  private:
    int lsock;
    int port;
  public:
    tcpServer(int _port = 8080) :port(_port) {}

    ~tcpServer(){close(lsock);}

    void tcpServerInit()
    {
        lsock = Sock::Socket();
        Sock::Setsockopt(lsock);
        Sock::Bind(lsock, port);
        Sock::Listen(lsock);
    }

    int getLsock()
    {
        return lsock;
    }
};
