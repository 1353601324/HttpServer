#pragma once
#include <iostream>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "log.hpp"

using namespace std;

#define BACKLOG 5

class Sock
{
  public:
    static int Socket()
    {  
        return socket(AF_INET, SOCK_STREAM, 0);
    }

    static void Bind(int sock, int port)
    {
        struct sockaddr_in local;
        local.sin_family = AF_INET;
        local.sin_port = htons(port);
        local.sin_addr.s_addr = htonl(INADDR_ANY);
        if(bind(sock, (struct sockaddr*)&local, sizeof local) < 0)
        {
            LOG("ERROR", "bind error");
            exit(1);
        }
    }

    static void Setsockopt(int sock)
    {
        int opt = 1;
        setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof opt);
    }

    static void Listen(int sock)
    {
        if(listen(sock, BACKLOG) < 0)
        {
            LOG("ERROR", "listen error");
            exit(2);
        }
    }

    static int Accept(int lsock)
    {
        struct sockaddr_in peer;
        socklen_t addrlen = sizeof peer;
        int sock = accept(lsock, (struct sockaddr*)&peer, &addrlen);
        if(sock < 0)
        {
            LOG("ERROR", "accept error");
            exit(3);
        }
        return sock;
    }
};
